// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Headless tests for the QJSEngine plugin host: manifest validation, the data
// API over a real (throwaway) database, permissions, menus, settings, timers,
// the watchdog, files/storage rules, change events, installing packages and
// approval. Plugins report results through ui.message, which the fake
// services record.

#include "ProjectNotesPlugins/PluginManifest.h"
#include "ProjectNotesPlugins/PluginRegistry.h"
#include "ProjectNotesPlugins/PluginServices.h"
#include "ProjectNotesPlugins/PublishedSchema.h"

#include "databaseobjects.h"

#include <QCoreApplication>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <private/qzipwriter_p.h>

using namespace PN::Plugins;

namespace {

class FakeServices final : public PluginServices {
public:
    struct Call { QString plugin; QString method; QVariantList args; };
    QList<Call> calls;
    QVariant nextConfirm = true;

    void request(const PluginManifest& plugin, const QString& method, const QVariantList& args, Reply reply) override
    {
        calls.append({plugin.id, method, args});
        if (method == QLatin1String("ui.confirm"))
            reply(nextConfirm, {});
        else if (method == QLatin1String("secrets.get"))
            reply(QStringLiteral("s3cret"), {});
        else
            reply(QVariant::fromValue(nullptr), {});
    }
    QStringList capabilities() const override { return {QStringLiteral("email.compose")}; }

    // The JSON payload of the most recent ui.message from `plugin`.
    QJsonObject lastMessage(const QString& plugin) const
    {
        for (auto it = calls.crbegin(); it != calls.crend(); ++it)
            if (it->plugin == plugin && it->method == QLatin1String("ui.message"))
                return QJsonDocument::fromJson(it->args.value(0).toString().toUtf8()).object();
        return {};
    }
    int messageCount(const QString& plugin) const
    {
        int n = 0;
        for (const Call& c : calls)
            if (c.plugin == plugin && c.method == QLatin1String("ui.message"))
                ++n;
        return n;
    }
};

void writeFile(const QString& path, const QByteArray& content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content);
}

QByteArray manifest(const QString& id, const QStringList& permissions, const QString& extra = {})
{
    QStringList quoted;
    for (const QString& p : permissions)
        quoted.append(QLatin1Char('"') + p + QLatin1Char('"'));
    return QStringLiteral(R"({"id": "%1", "name": "%1", "version": "1.0.0", "apiVersion": 1,
        "description": "test", "main": "main.mjs", "permissions": [%2]%3})")
        .arg(id, quoted.join(QLatin1Char(',')), extra).toUtf8();
}

QByteArray fixture(const QString& name)
{
    QFile file(QStringLiteral(PN_TEST_FIXTURES) + QStringLiteral("/plugins/") + name);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

} // namespace

class PluginHostTest final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void manifestValidation();
    void publishedSchemaMapping();
    void listUsesPublishedNames();
    void menuRecordContext();
    void queryWithIncludes();
    void whereOperators();
    void saveInsertsThenUpdates();
    void saveRejections_data();
    void saveRejections();
    void transactions();
    void removeRules();
    void schemaDescribesEntity();
    void settingsAndStorage();
    void filesAndHttpPermissions();
    void timersAndCommonHelpers();
    void appInfoAndSecrets();
    void dynamicMenus();
    void watchdogStopsRunawayCode();
    void exportXml();
    void changeEventsReachOtherPlugins();
    void userPluginNeedsApproval();
    void installPackage();
    void hotReload();

private:
    QJsonObject run(const QString& menuId, const QString& entity = {}, const QString& key = {}, int timeoutMs = 10000);
    void startProbe();

    QTemporaryDir m_dir;
    QString m_database;
    QString m_bundled;
    QString m_user;
    FakeServices m_services;
    std::unique_ptr<PluginRegistry> m_registry;
};

void PluginHostTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QVERIFY(m_dir.isValid());
    const QString empty = m_dir.filePath(QStringLiteral("empty.db"));
    QVERIFY(global_DBObjects.createDatabase(empty));
    m_database = m_dir.filePath(QStringLiteral("plugins.db"));
    QVERIFY(QFile::copy(empty, m_database));

    DatabaseObjects seed(nullptr);
    QVERIFY(seed.openDatabase(m_database, QStringLiteral("plugin-test-seed"), false));
    QFile importFile(QStringLiteral(PN_TEST_FIXTURES) + QStringLiteral("/recordtree/import.xml"));
    QVERIFY(importFile.open(QIODevice::ReadOnly));
    QDomDocument document;
    QVERIFY(document.setContent(importFile.readAll()));
    QVERIFY(seed.importXMLDoc(document));
    seed.closeDatabase();

    m_bundled = m_dir.filePath(QStringLiteral("bundled"));
    m_user = m_dir.filePath(QStringLiteral("user"));
    QDir().mkpath(m_bundled);
    QDir().mkpath(m_user);

    QSettings(QStringLiteral("ProjectNotesPluginTest"), QStringLiteral("PluginState")).clear();
    QSettings(QStringLiteral("ProjectNotesPluginTest"), QStringLiteral("PluginSettings")).clear();

    writeFile(m_bundled + QStringLiteral("/probe/plugin.json"),
              manifest(QStringLiteral("probe"),
                       {QStringLiteral("data.read:companies,contacts,projects,project_team,item_tracker,item_tracker_updates,project_notes"),
                        QStringLiteral("data.write:companies,contacts,item_tracker"), QStringLiteral("files")},
                       QStringLiteral(R"(, "contributes": {"menus": [{"id": "project", "title": "Probe", "context": "record",
                                       "entity": "projects", "include": ["project_team"]}]})")));
    writeFile(m_bundled + QStringLiteral("/probe/main.mjs"), fixture(QStringLiteral("probe.mjs")));

    m_registry = std::make_unique<PluginRegistry>(&m_services);
    m_registry->setPaths({m_bundled, m_user, m_dir.filePath(QStringLiteral("data")), m_dir.filePath(QStringLiteral("temp"))});
    m_registry->setDatabasePath(m_database);
    m_registry->setSettingsOrganization(QStringLiteral("ProjectNotesPluginTest"));
    m_registry->setAppVersion(QStringLiteral("6.4.0"));
    m_registry->setWatchdogBudgetMs(1500);
    startProbe();
}

void PluginHostTest::startProbe()
{
    m_registry->loadAll();
    QTRY_VERIFY2_WITH_TIMEOUT(m_registry->isRunning(QStringLiteral("probe")),
                              qPrintable(m_registry->plugin(QStringLiteral("probe")).loadError + QStringLiteral(" | ")
                                         + m_registry->plugin(QStringLiteral("probe")).lastError), 10000);
}

void PluginHostTest::cleanupTestCase()
{
    m_registry.reset();
}

QJsonObject PluginHostTest::run(const QString& menuId, const QString& entity, const QString& key, int timeoutMs)
{
    const int before = m_services.messageCount(QStringLiteral("probe"));
    m_registry->runMenu(QStringLiteral("probe"), menuId, entity, key);
    QElapsedTimer timer;
    timer.start();
    while (m_services.messageCount(QStringLiteral("probe")) == before && timer.elapsed() < timeoutMs)
        QTest::qWait(10);
    const QJsonObject message = m_services.lastMessage(QStringLiteral("probe"));
    if (m_services.messageCount(QStringLiteral("probe")) == before)
        return {};
    return message;
}

void PluginHostTest::manifestValidation()
{
    auto check = [this](const QByteArray& json, bool expectOk, const QString& expectIn = {}) {
        const QString folder = m_dir.filePath(QStringLiteral("manifest-%1").arg(QRandomGenerator::global()->generate()));
        writeFile(folder + QStringLiteral("/plugin.json"), json);
        writeFile(folder + QStringLiteral("/main.mjs"), "export function onStart() {}");
        PluginManifest m;
        QString error;
        const bool ok = PluginManifest::load(folder, &m, &error);
        QVERIFY2(ok == expectOk, qPrintable(error));
        if (!expectIn.isEmpty())
            QVERIFY2(error.contains(expectIn), qPrintable(error));
    };
    check(manifest(QStringLiteral("good.plugin"), {QStringLiteral("data.read:contacts")}), true);
    check(manifest(QStringLiteral("bad id!"), {}), false, QStringLiteral("id"));
    check(manifest(QStringLiteral("stored"), {QStringLiteral("data.read:people")}), false, QStringLiteral("contacts"));
    check(manifest(QStringLiteral("menu"), {}, QStringLiteral(R"(, "contributes": {"menus": [{"id": "m", "title": "M", "context": "record", "entity": "clients"}]})")),
          false, QStringLiteral("companies"));
    check(QByteArray(R"({"id": "v2", "name": "v2", "apiVersion": 2, "main": "main.mjs"})"), false, QStringLiteral("apiVersion"));

    PluginManifest m;
    QString error;
    const QString folder = m_dir.filePath(QStringLiteral("perm-check"));
    writeFile(folder + QStringLiteral("/plugin.json"),
              manifest(QStringLiteral("perm.check"), {QStringLiteral("data.read:contacts"), QStringLiteral("data.write:companies"), QStringLiteral("com")},
                       QStringLiteral(R"(, "http": ["*.ifs.cloud"], "com": ["Word.Application", "winmgmts:"])")));
    writeFile(folder + QStringLiteral("/main.mjs"), "");
    QVERIFY(PluginManifest::load(folder, &m, &error));
    QVERIFY(m.canRead(QStringLiteral("contacts")));
    QVERIFY(m.canRead(QStringLiteral("companies")));      // write implies read
    QVERIFY(!m.canWrite(QStringLiteral("contacts")));
    QVERIFY(m.allowsHost(QStringLiteral("acme.ifs.cloud")));
    QVERIFY(!m.allowsHost(QStringLiteral("ifs.cloud.evil.example")));
    QVERIFY(m.allowsComObject(QStringLiteral("word.application")));
    QVERIFY(m.allowsComObject(QStringLiteral("winmgmts:{impersonationLevel=impersonate}")));
    QVERIFY(!m.allowsComObject(QStringLiteral("Excel.Application")));
}

void PluginHostTest::publishedSchemaMapping()
{
    const PublishedSchema& s = PublishedSchema::instance();
    QCOMPARE(s.storedTable(QStringLiteral("contacts")), QStringLiteral("people"));
    QCOMPARE(s.storedTable(QStringLiteral("project_team")), QStringLiteral("project_people"));
    QCOMPARE(s.publishedEntity(QStringLiteral("clients")), QStringLiteral("companies"));
    QCOMPARE(s.publishedColumn(QStringLiteral("person_id")), QStringLiteral("contact_id"));
    QCOMPARE(s.publishedColumn(QStringLiteral("client_name")), QStringLiteral("company_name"));
    QCOMPARE(s.publishedValue(QStringLiteral("people_source"), QStringLiteral("chosen-people")), QStringLiteral("chosen-contacts"));
    QVERIFY(s.isRetiredName(QStringLiteral("people")));
    QVERIFY(!s.isEntity(QStringLiteral("people")));
}

void PluginHostTest::listUsesPublishedNames()
{
    const QJsonObject r = run(QStringLiteral("companies"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonArray companies = r.value(QStringLiteral("result")).toArray();
    QVERIFY(companies.size() >= 2);
    const QJsonObject acme = companies.at(0).toObject();
    QCOMPARE(acme.value(QStringLiteral("$entity")).toString(), QStringLiteral("companies"));
    QCOMPARE(acme.value(QStringLiteral("company_name")).toString(), QStringLiteral("Acme Corp"));
    QCOMPARE(acme.value(QStringLiteral("$key")).toString(), QStringLiteral("client-acme"));
    QVERIFY(!acme.contains(QStringLiteral("client_name")));
}

void PluginHostTest::menuRecordContext()
{
    const QVariantList menus = m_registry->menusForEntity(QStringLiteral("projects"));
    QCOMPARE(menus.size(), 1);
    QCOMPARE(menus.first().toMap().value(QStringLiteral("menuId")).toString(), QStringLiteral("project"));

    const QJsonObject r = run(QStringLiteral("project"), QStringLiteral("projects"), QStringLiteral("project-p4000"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject project = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(project.value(QStringLiteral("project_number")).toString(), QStringLiteral("P4000"));
    QCOMPARE(project.value(QStringLiteral("company_id")).toObject().value(QStringLiteral("lookup")).toString(), QStringLiteral("Acme Corp"));
    QCOMPARE(project.value(QStringLiteral("last_status_date")).toString(), QStringLiteral("2026-09-02"));
    QCOMPARE(project.value(QStringLiteral("budget")).toDouble(), 2090.0);
    const QJsonArray team = project.value(QStringLiteral("project_team")).toArray();
    QCOMPARE(team.size(), 2);
    QStringList names;
    for (const QJsonValue& member : team)
        names.append(member.toObject().value(QStringLiteral("contact_id")).toObject().value(QStringLiteral("lookup")).toString());
    names.sort();
    QCOMPARE(names, (QStringList{QStringLiteral("Jim Smith"), QStringLiteral("Mike Smith")}));
    QVERIFY(!project.contains(QStringLiteral("item_tracker")));    // not in the menu's include
}

void PluginHostTest::queryWithIncludes()
{
    const QJsonObject r = run(QStringLiteral("projectQuery"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject project = r.value(QStringLiteral("result")).toObject();
    const QJsonArray team = project.value(QStringLiteral("project_team")).toArray();
    QCOMPARE(team.size(), 1);   // include where: role = Lead
    QCOMPARE(team.at(0).toObject().value(QStringLiteral("role")).toString(), QStringLiteral("Lead"));
    QVERIFY(project.value(QStringLiteral("item_tracker")).toArray().size() >= 2);
}

void PluginHostTest::whereOperators()
{
    const QJsonObject r = run(QStringLiteral("filtered"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject result = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("open")).toArray(), (QJsonArray{QStringLiteral("2"), QStringLiteral("1")}));
    QCOMPARE(result.value(QStringLiteral("dated")).toInt(), 1);
    QCOMPARE(result.value(QStringLiteral("byLookup")).toInt(), 2);
}

void PluginHostTest::saveInsertsThenUpdates()
{
    const QJsonObject r = run(QStringLiteral("saveContact"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject result = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("first")).toString(), QStringLiteral("insert"));
    QCOMPARE(result.value(QStringLiteral("second")).toString(), QStringLiteral("update"));   // matched by the "name" unique key
    QVERIFY(result.value(QStringLiteral("sameKey")).toBool());
    QCOMPARE(result.value(QStringLiteral("company")).toObject().value(QStringLiteral("lookup")).toString(), QStringLiteral("Globex, Inc."));
    QCOMPARE(result.value(QStringLiteral("email")).toString(), QStringLiteral("zoe2@example.com"));
}

void PluginHostTest::saveRejections_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<QString>("code");
    QTest::addColumn<QString>("permission");
    QTest::newRow("read-only column") << "readOnly" << "validation-failed" << "";
    QTest::newRow("stored entity name") << "storedName" << "validation-failed" << "";
    QTest::newRow("unknown lookup") << "badLookup" << "validation-failed" << "";
    QTest::newRow("undeclared entity") << "notPermitted" << "permission-denied" << "data.read:status_report_items";
    QTest::newRow("value not in list") << "badChoice" << "validation-failed" << "";
}

void PluginHostTest::saveRejections()
{
    QFETCH(QString, scenario);
    QFETCH(QString, code);
    QFETCH(QString, permission);
    const QJsonObject r = run(scenario);
    QVERIFY(!r.isEmpty());
    QVERIFY2(!r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject error = r.value(QStringLiteral("error")).toObject();
    QCOMPARE(error.value(QStringLiteral("code")).toString(), code);
    if (!permission.isEmpty())
        QCOMPARE(error.value(QStringLiteral("permission")).toString(), permission);
}

void PluginHostTest::transactions()
{
    QJsonObject r = run(QStringLiteral("txRollback"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    QCOMPARE(r.value(QStringLiteral("result")).toInt(), 0);

    r = run(QStringLiteral("txCommit"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    QCOMPARE(r.value(QStringLiteral("result")).toObject().value(QStringLiteral("lookup")).toString(), QStringLiteral("Committed Co"));
    QCOMPARE(r.value(QStringLiteral("result")).toObject().value(QStringLiteral("count")).toInt(), 1);
}

void PluginHostTest::removeRules()
{
    QJsonObject r = run(QStringLiteral("removeBlocked"));
    QVERIFY(!r.value(QStringLiteral("ok")).toBool());
    QCOMPARE(r.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("reference-blocked"));

    r = run(QStringLiteral("removeCascade"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonArray removed = r.value(QStringLiteral("result")).toArray();
    QVERIFY(removed.contains(QStringLiteral("item_tracker")));
}

void PluginHostTest::schemaDescribesEntity()
{
    const QJsonObject r = run(QStringLiteral("schema"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject s = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(s.value(QStringLiteral("key")).toString(), QStringLiteral("id"));
    QVERIFY(s.value(QStringLiteral("columns")).toArray().contains(QStringLiteral("contact_id")));
    QVERIFY(s.value(QStringLiteral("columns")).toArray().contains(QStringLiteral("company_name")));
    QCOMPARE(s.value(QStringLiteral("contact")).toString(), QStringLiteral("ref"));
    QCOMPARE(s.value(QStringLiteral("ref")).toString(), QStringLiteral("contacts"));
}

void PluginHostTest::settingsAndStorage()
{
    QJsonObject r = run(QStringLiteral("settingsRoundTrip"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    QJsonObject result = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("color")).toString(), QStringLiteral("blue"));
    QCOMPARE(result.value(QStringLiteral("missing")).toString(), QStringLiteral("dflt"));
    QCOMPARE(result.value(QStringLiteral("nested")).toObject().value(QStringLiteral("a")).toArray().size(), 2);
    QSettings stored(QStringLiteral("ProjectNotesPluginTest"), QStringLiteral("PluginSettings"));
    QCOMPARE(stored.value(QStringLiteral("probe/color")).toString(), QStringLiteral("blue"));

    r = run(QStringLiteral("storageRoundTrip"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    result = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("back")).toObject().value(QStringLiteral("n")).toInt(), 3);
    QCOMPARE(result.value(QStringLiteral("escaped")).toString(), QStringLiteral("validation-failed"));
    QCOMPARE(result.value(QStringLiteral("list")).toArray(), (QJsonArray{QStringLiteral("last.json")}));
}

void PluginHostTest::filesAndHttpPermissions()
{
    QJsonObject r = run(QStringLiteral("filesDenied"));
    QVERIFY(!r.value(QStringLiteral("ok")).toBool());
    QCOMPARE(r.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("permission-denied"));

    r = run(QStringLiteral("filesPackage"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    QCOMPARE(r.value(QStringLiteral("result")).toString(), QStringLiteral("probe"));

    r = run(QStringLiteral("httpDenied"));
    QVERIFY(!r.value(QStringLiteral("ok")).toBool());
    QCOMPARE(r.value(QStringLiteral("error")).toObject().value(QStringLiteral("permission")).toString(), QStringLiteral("http"));
}

void PluginHostTest::timersAndCommonHelpers()
{
    QJsonObject r = run(QStringLiteral("timer"));
    QCOMPARE(r.value(QStringLiteral("result")).toString(), QStringLiteral("fired"));

    r = run(QStringLiteral("common"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject result = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("vars")).toString(), QStringLiteral("x P4000 Acme Corp Lead "));
    QCOMPARE(result.value(QStringLiteral("date")).toString(), QStringLiteral("09/02/2026"));
    QCOMPARE(result.value(QStringLiteral("parsed")).toString(), QStringLiteral("2026-09-02"));
    QCOMPARE(result.value(QStringLiteral("html")).toString(), QStringLiteral("&lt;a&amp;b&gt;"));
}

void PluginHostTest::appInfoAndSecrets()
{
    const QJsonObject r = run(QStringLiteral("info"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject result = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("id")).toString(), QStringLiteral("probe"));
    QVERIFY(result.value(QStringLiteral("email")).toBool());
#ifndef Q_OS_WIN
    QVERIFY(!result.value(QStringLiteral("com")).toBool());
#endif
    QCOMPARE(result.value(QStringLiteral("secret")).toString(), QStringLiteral("s3cret"));
}

void PluginHostTest::dynamicMenus()
{
    QSignalSpy changed(m_registry.get(), &PluginRegistry::menusChanged);
    QJsonObject r = run(QStringLiteral("dynamicMenu"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    QVERIFY(r.value(QStringLiteral("result")).toArray().contains(QStringLiteral("dyn")));
    QTRY_VERIFY(changed.count() > 0);
    QTRY_COMPARE(m_registry->menusForEntity(QStringLiteral("contacts")).size(), 1);

    r = run(QStringLiteral("dyn"), QStringLiteral("contacts"), QStringLiteral("person-mike"));
    QVERIFY2(r.value(QStringLiteral("menuId")).toString() == QLatin1String("dyn"),
             qPrintable(m_registry->plugin(QStringLiteral("probe")).lastError));
    QCOMPARE(r.value(QStringLiteral("result")).toString(), QStringLiteral("Mike Smith"));
}

void PluginHostTest::watchdogStopsRunawayCode()
{
    QSignalSpy errors(m_registry.get(), &PluginRegistry::pluginError);
    m_registry->runMenu(QStringLiteral("probe"), QStringLiteral("runaway"), {}, {});
    QTRY_VERIFY_WITH_TIMEOUT(errors.count() > 0, 8000);
    QVERIFY(errors.last().at(1).toString().contains(QStringLiteral("stopped")));
    // The plugin keeps working afterwards.
    const QJsonObject r = run(QStringLiteral("companies"));
    QVERIFY(r.value(QStringLiteral("ok")).toBool());
}

void PluginHostTest::exportXml()
{
    const QJsonObject r = run(QStringLiteral("xml"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    QVERIFY(r.value(QStringLiteral("result")).toObject().value(QStringLiteral("hasClients")).toBool());
    QVERIFY(r.value(QStringLiteral("result")).toObject().value(QStringLiteral("hasName")).toBool());
}

void PluginHostTest::changeEventsReachOtherPlugins()
{
    // A second bundled plugin listens for contact changes and reports them.
    writeFile(m_bundled + QStringLiteral("/listener/plugin.json"),
              manifest(QStringLiteral("listener"), {QStringLiteral("data.read:contacts")}));
    writeFile(m_bundled + QStringLiteral("/listener/main.mjs"), fixture(QStringLiteral("listener.mjs")));
    QSignalSpy pluginData(m_registry.get(), &PluginRegistry::pluginDataChanged);
    startProbe();
    QTRY_VERIFY_WITH_TIMEOUT(m_registry->isRunning(QStringLiteral("listener")), 10000);

    const QJsonObject r = run(QStringLiteral("saveContact"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson() + m_registry->plugin(QStringLiteral("probe")).lastError));
    QTRY_VERIFY(pluginData.count() > 0);
    QTRY_VERIFY_WITH_TIMEOUT(m_services.messageCount(QStringLiteral("listener")) > 0, 5000);
    const QJsonObject change = m_services.lastMessage(QStringLiteral("listener"));
    QCOMPARE(change.value(QStringLiteral("entity")).toString(), QStringLiteral("contacts"));
    QCOMPARE(change.value(QStringLiteral("source")).toString(), QStringLiteral("plugin"));
    QCOMPARE(change.value(QStringLiteral("pluginId")).toString(), QStringLiteral("probe"));

    // Changes the application reports reach plugins as source "app".
    m_registry->notifyDataChanged({QVariantMap{{QStringLiteral("table"), QStringLiteral("people")},
                                               {QStringLiteral("key"), QStringLiteral("person-ann")},
                                               {QStringLiteral("operation"), 1}}});
    QTRY_COMPARE(m_services.lastMessage(QStringLiteral("listener")).value(QStringLiteral("source")).toString(), QStringLiteral("app"));
    QCOMPARE(m_services.lastMessage(QStringLiteral("listener")).value(QStringLiteral("operation")).toString(), QStringLiteral("update"));
}

void PluginHostTest::userPluginNeedsApproval()
{
    writeFile(m_user + QStringLiteral("/user.tool/plugin.json"),
              manifest(QStringLiteral("user.tool"), {QStringLiteral("data.read:contacts")}));
    writeFile(m_user + QStringLiteral("/user.tool/main.mjs"), "import { ui } from 'projectnotes'; export function onStart() { ui.message('{\"up\":true}'); }");
    startProbe();
    PluginInfo info = m_registry->plugin(QStringLiteral("user.tool"));
    QVERIFY(info.valid);
    QVERIFY(info.needsApproval);
    QVERIFY(!info.enabled);
    QVERIFY(!m_registry->isRunning(QStringLiteral("user.tool")));
    QString error;
    QVERIFY(!m_registry->setEnabled(QStringLiteral("user.tool"), true, &error));

    QVERIFY(m_registry->approve(QStringLiteral("user.tool"), &error));
    QTRY_VERIFY_WITH_TIMEOUT(m_registry->isRunning(QStringLiteral("user.tool")), 10000);
    QTRY_VERIFY(m_services.messageCount(QStringLiteral("user.tool")) > 0);

    // Changing the declared permissions requires approval again.
    writeFile(m_user + QStringLiteral("/user.tool/plugin.json"),
              manifest(QStringLiteral("user.tool"), {QStringLiteral("data.read:contacts"), QStringLiteral("data.write:contacts")}));
    m_registry->reload(QStringLiteral("user.tool"));
    info = m_registry->plugin(QStringLiteral("user.tool"));
    QVERIFY(info.needsApproval);
    QVERIFY(!m_registry->isRunning(QStringLiteral("user.tool")));
}

void PluginHostTest::installPackage()
{
    const QString zipPath = m_dir.filePath(QStringLiteral("packaged.pnplugin"));
    {
        QZipWriter zip(zipPath);
        zip.addFile(QStringLiteral("packaged/plugin.json"), manifest(QStringLiteral("packaged.tool"), {}));
        zip.addFile(QStringLiteral("packaged/main.mjs"), "export function onStart() {}");
        zip.close();
    }
    PluginManifest inspected;
    QString error;
    QVERIFY2(m_registry->inspectPackage(zipPath, &inspected, &error), qPrintable(error));
    QCOMPARE(inspected.id, QStringLiteral("packaged.tool"));

    QString id;
    QVERIFY2(m_registry->install(zipPath, &id, &error), qPrintable(error));
    QCOMPARE(id, QStringLiteral("packaged.tool"));
    QVERIFY(QFileInfo::exists(m_user + QStringLiteral("/packaged.tool/plugin.json")));
    QVERIFY(m_registry->plugin(id).needsApproval);

    // Packages with paths that escape the folder are refused.
    const QString evilPath = m_dir.filePath(QStringLiteral("evil.pnplugin"));
    {
        QZipWriter zip(evilPath);
        zip.addFile(QStringLiteral("../escape/plugin.json"), manifest(QStringLiteral("evil"), {}));
        zip.close();
    }
    // Depending on how the archive reader normalizes the name, the install is
    // refused as unsafe or as an invalid package; either way nothing escapes.
    QVERIFY(!m_registry->install(evilPath, &id, &error));
    QVERIFY(!QFileInfo::exists(m_dir.filePath(QStringLiteral("escape"))));
    QVERIFY(!QFileInfo::exists(QDir(m_user).filePath(QStringLiteral("../escape"))));

    QVERIFY2(m_registry->uninstall(QStringLiteral("packaged.tool"), &error), qPrintable(error));
    QVERIFY(!QFileInfo::exists(m_user + QStringLiteral("/packaged.tool")));
}

void PluginHostTest::hotReload()
{
    writeFile(m_bundled + QStringLiteral("/reloader/plugin.json"), manifest(QStringLiteral("reloader"), {}));
    writeFile(m_bundled + QStringLiteral("/reloader/main.mjs"),
              "import { ui } from 'projectnotes'; export function onStart() { ui.message('{\"v\":1}'); }");
    startProbe();
    QTRY_COMPARE_WITH_TIMEOUT(m_services.lastMessage(QStringLiteral("reloader")).value(QStringLiteral("v")).toInt(), 1, 10000);

    writeFile(m_bundled + QStringLiteral("/reloader/main.mjs"),
              "import { ui } from 'projectnotes'; export function onStart() { ui.message('{\"v\":2}'); }");
    QTRY_COMPARE_WITH_TIMEOUT(m_services.lastMessage(QStringLiteral("reloader")).value(QStringLiteral("v")).toInt(), 2, 10000);
}

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    PluginHostTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_pluginhost.moc"

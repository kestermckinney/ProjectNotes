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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <private/qzipwriter_p.h>

#include <functional>

using namespace PN::Plugins;

namespace {

class FakeServices final : public PluginServices {
public:
    struct Call { QString plugin; QString method; QVariantList args; };
    QList<Call> calls;
    QVariant nextConfirm = true;
    QHash<QString, QVariant> replies;       // canned replies by method
    QStringList capabilityList{QStringLiteral("email.compose")};
    // Answers a call itself when it returns true.
    std::function<bool(const QString& method, const QVariantList& args, const Reply& reply)> handler;

    void request(const PluginManifest& plugin, const QString& method, const QVariantList& args, Reply reply) override
    {
        calls.append({plugin.id, method, args});
        if (handler && handler(method, args, reply))
            return;
        if (replies.contains(method))
            reply(replies.value(method), {});
        else if (method == QLatin1String("ui.confirm"))
            reply(nextConfirm, {});
        else if (method == QLatin1String("secrets.get"))
            reply(QStringLiteral("s3cret"), {});
        else
            reply(QVariant::fromValue(nullptr), {});
    }
    QStringList capabilities() const override { return capabilityList; }

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
    // Arguments of the calls to `method` from `plugin`, oldest first.
    QList<QVariantList> argsOf(const QString& plugin, const QString& method) const
    {
        QList<QVariantList> out;
        for (const Call& c : calls)
            if (c.plugin == plugin && c.method == method)
                out.append(c.args);
        return out;
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

// A tiny HTTP/1.1 server for plugin tests: one request per connection.
class FakeHttpServer final : public QObject {
public:
    struct Request { QByteArray method; QString target; QByteArray body; QByteArray authorization; };
    using Handler = std::function<QPair<int, QByteArray>(const Request&)>;
    QList<Request> requests;

    explicit FakeHttpServer(Handler handler) : m_handler(std::move(handler))
    {
        QObject::connect(&m_server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket* socket = m_server.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                QObject::connect(socket, &QTcpSocket::readyRead, this, [this, socket] { serve(socket); });
            }
        });
        m_server.listen(QHostAddress::LocalHost);
    }
    quint16 port() const { return m_server.serverPort(); }
    QString base() const { return QStringLiteral("http://127.0.0.1:%1").arg(port()); }

private:
    void serve(QTcpSocket* socket)
    {
        const QByteArray data = socket->peek(1 << 20);
        const int headEnd = data.indexOf("\r\n\r\n");
        if (headEnd < 0)
            return;
        const QByteArray head = data.left(headEnd);
        int length = 0;
        QByteArray authorization;
        for (const QByteArray& line : head.split('\n')) {
            const QByteArray l = line.trimmed();
            if (l.toLower().startsWith("content-length:"))
                length = l.mid(15).trimmed().toInt();
            if (l.toLower().startsWith("authorization:"))
                authorization = l.mid(14).trimmed();
        }
        if (data.size() < headEnd + 4 + length)
            return;
        socket->readAll();
        const QList<QByteArray> requestLine = head.left(head.indexOf("\r\n")).split(' ');
        Request request{requestLine.value(0), QUrl::fromPercentEncoding(requestLine.value(1)),
                        data.mid(headEnd + 4, length), authorization};
        requests.append(request);
        const auto [status, body] = m_handler(request);
        socket->write("HTTP/1.1 " + QByteArray::number(status) + " X\r\nContent-Type: application/json\r\nConnection: close\r\n"
                      "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
        socket->disconnectFromHost();
    }

    QTcpServer m_server;
    Handler m_handler;
};

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
    void projectFolderReference();
    void saveRejections_data();
    void saveRejections();
    void transactions();
    void removeRules();
    void schemaDescribesEntity();
    void settingsAndStorage();
    void filesAndHttpPermissions();
    void browserSignIn();
    void timersAndCommonHelpers();
    void appInfoAndSecrets();
    void recordsUseNativeArrays();
    void dynamicMenus();
    void watchdogStopsRunawayCode();
    void exportXml();
    void changeEventsReachOtherPlugins();
    void userPluginNeedsApproval();
    void installPackage();
    void hotReload();
    void bundledPluginsRun();
    void ifsCloudPlugin();
    void ifsCloudDefaultShortcuts();

private:
    QJsonObject run(const QString& menuId, const QString& entity = {}, const QString& key = {}, int timeoutMs = 10000);
    void startProbe();

    QTemporaryDir m_dir;
    QString m_database;
    QString m_pristine;
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
    m_pristine = m_dir.filePath(QStringLiteral("pristine.db"));
    QVERIFY(QFile::copy(m_database, m_pristine));   // for tests that need the fixture unchanged

    m_bundled = m_dir.filePath(QStringLiteral("bundled"));
    m_user = m_dir.filePath(QStringLiteral("user"));
    QDir().mkpath(m_bundled);
    QDir().mkpath(m_user);

    QSettings(QStringLiteral("ProjectNotesPluginTest"), QStringLiteral("PluginState")).clear();
    QSettings(QStringLiteral("ProjectNotesPluginTest"), QStringLiteral("PluginSettings")).clear();

    writeFile(m_bundled + QStringLiteral("/probe/plugin.json"),
              manifest(QStringLiteral("probe"),
                       {QStringLiteral("data.read:companies,contacts,projects,project_team,item_tracker,item_tracker_updates,project_notes"),
                        QStringLiteral("data.write:companies,contacts,projects,item_tracker"), QStringLiteral("files"),
                        QStringLiteral("auth.browser")},
                       QStringLiteral(R"(, "http": ["sso.example.test"], "contributes": {"menus": [{"id": "project", "title": "Probe", "context": "record",
                                       "entity": "projects", "include": ["project_team"]}]})")));
    writeFile(m_bundled + QStringLiteral("/probe/main.mjs"), fixture(QStringLiteral("probe.mjs")));

    m_registry = std::make_unique<PluginRegistry>(&m_services);
    m_registry->setPaths({m_bundled, m_user, m_dir.filePath(QStringLiteral("data")), m_dir.filePath(QStringLiteral("temp"))});
    m_registry->setDatabasePath(m_database);
    m_registry->setSettingsOrganization(QStringLiteral("ProjectNotesPluginTest"));
    m_registry->setAppVersion(QStringLiteral("6.5.0"));
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

void PluginHostTest::projectFolderReference()
{
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("folder-seed"));
        db.setDatabaseName(m_database);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY2(q.exec(QStringLiteral("INSERT INTO project_folders(id, folder_name, icon, color, sort_order, deleted) "
                                       "VALUES('folder-fav', 'Favorites', 'star', '#c98a1a', 0, 0)")), qPrintable(q.lastError().text()));
        q.finish();
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("folder-seed"));

    const QJsonObject r = run(QStringLiteral("projectFolder"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject result = r.value(QStringLiteral("result")).toObject();
    QVERIFY(result.value(QStringLiteral("before")).isNull());
    QCOMPARE(result.value(QStringLiteral("inFolder")).toInt(), 1);
    QCOMPARE(result.value(QStringLiteral("moved")).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("folder-fav"));
    QCOMPARE(result.value(QStringLiteral("moved")).toObject().value(QStringLiteral("lookup")).toString(), QStringLiteral("Favorites"));
    QVERIFY(result.value(QStringLiteral("cleared")).isNull());
    QCOMPARE(result.value(QStringLiteral("unknown")).toString(), QStringLiteral("validation-failed"));
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

void PluginHostTest::browserSignIn()
{
    QTcpServer probePort;
    QVERIFY(probePort.listen(QHostAddress::LocalHost));
    const quint16 port = probePort.serverPort();
    probePort.close();
    QSettings(QStringLiteral("ProjectNotesPluginTest"), QStringLiteral("PluginSettings"))
        .setValue(QStringLiteral("probe/signInPort"), port);

    const int before = m_services.messageCount(QStringLiteral("probe"));
    const int opened = m_services.argsOf(QStringLiteral("probe"), QStringLiteral("ui.openUrl")).size();
    m_registry->runMenu(QStringLiteral("probe"), QStringLiteral("browserSignIn"), {}, {});
    QTRY_COMPARE(m_services.argsOf(QStringLiteral("probe"), QStringLiteral("ui.openUrl")).size(), opened + 1);
    QCOMPARE(m_services.argsOf(QStringLiteral("probe"), QStringLiteral("ui.openUrl")).last().value(0).toString(),
             QStringLiteral("https://sso.example.test/auth?client_id=probe"));

    // The browser asks for other paths first; those get an empty answer.
    auto request = [port](const QByteArray& path) {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, port);
        if (!socket.waitForConnected(3000))
            return QByteArray("no connection");
        socket.write("GET " + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
        QByteArray response;
        QElapsedTimer timer;
        timer.start();
        while (socket.state() == QAbstractSocket::ConnectedState && timer.elapsed() < 3000) {
            QCoreApplication::processEvents();
            if (socket.waitForReadyRead(50))
                response += socket.readAll();
        }
        return response + socket.readAll();
    };
    QVERIFY(request("/favicon.ico").startsWith("HTTP/1.1 204"));
    QVERIFY(request("/callback?code=abc%2B1&state=s1").startsWith("HTTP/1.1 200"));
    QTRY_VERIFY(m_services.messageCount(QStringLiteral("probe")) > before);
    QJsonObject r = m_services.lastMessage(QStringLiteral("probe"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    QCOMPARE(r.value(QStringLiteral("result")).toString(),
             QStringLiteral("http://127.0.0.1:%1/callback?code=abc%2B1&state=s1").arg(port));

    r = run(QStringLiteral("browserSignInBadHost"));
    QCOMPARE(r.value(QStringLiteral("error")).toObject().value(QStringLiteral("permission")).toString(), QStringLiteral("http"));
    r = run(QStringLiteral("browserSignInBadRedirect"));
    QCOMPARE(r.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("validation-failed"));
}

void PluginHostTest::timersAndCommonHelpers()
{
    QJsonObject r = run(QStringLiteral("timer"));
    QCOMPARE(r.value(QStringLiteral("result")).toString(), QStringLiteral("fired"));

    r = run(QStringLiteral("common"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject result = r.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("vars")).toString(), QStringLiteral("x P4000 Acme Corp Lead "));
    QCOMPARE(result.value(QStringLiteral("tokens")).toString(), QStringLiteral("x P4000 Acme Corp Lead  09/02/2026 a@x.test, b@x.test|Lead|"));
    // The same cases tst_communicationcontracts.cpp runs against the C++ renderer.
    const QJsonArray grammar = result.value(QStringLiteral("grammar")).toArray();
    QCOMPARE(grammar.at(0).toString(), QStringLiteral("A<B|09/02/2026"));
    QCOMPARE(grammar.at(1).toString(), QStringLiteral("A&lt;B"));
    QCOMPARE(grammar.at(2).toString(), QStringLiteral("{{ project.name }}"));
    QCOMPARE(grammar.at(3).toString(), QString());
    QCOMPARE(grammar.at(4).toString(), QStringLiteral("a {{ project.name"));
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

void PluginHostTest::recordsUseNativeArrays()
{
    const QJsonObject r = run(QStringLiteral("arrays"));
    QVERIFY2(r.value(QStringLiteral("ok")).toBool(), qPrintable(QJsonDocument(r).toJson()));
    const QJsonObject result = r.value(QStringLiteral("result")).toObject();
    QVERIFY(result.value(QStringLiteral("isArray")).toBool());
    QCOMPARE(result.value(QStringLiteral("apply")).toInt(), 2);
    QVERIFY(result.value(QStringLiteral("vars")).toBool());
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

// The plugins that ship in the repository's plugins/ folder start cleanly and
// their menus work against the fixture database.
void PluginHostTest::bundledPluginsRun()
{
    const QString database = m_dir.filePath(QStringLiteral("bundled.db"));
    QVERIFY(QFile::copy(m_pristine, database));
    const QString organization = QStringLiteral("ProjectNotesBundledTest");
    QSettings(organization, QStringLiteral("PluginState")).clear();
    QSettings legacy(organization, QStringLiteral("PluginSettings"));
    legacy.clear();
    // A shortcut saved by the Python My Shortcuts plugin, with stored names.
    legacy.setValue(QStringLiteral("My Shortcuts/MyShortcuts"),
                    QString::fromUtf8(QJsonDocument(QJsonArray{QJsonObject{
                        {QStringLiteral("Menu"), QStringLiteral("Look Up")}, {QStringLiteral("Submenu"), QString()},
                        {QStringLiteral("URL"), QStringLiteral("https://example.test/?q=[$people.name.1]")},
                        {QStringLiteral("Data Type"), QStringLiteral("people")}}}).toJson(QJsonDocument::Compact)));
    legacy.setValue(QStringLiteral("New Risk Register/ExportSubFolder"), QStringLiteral("Risks"));
    legacy.setValue(QStringLiteral("Outlook Integration/IntegrationType"), QStringLiteral("Office 365 Application"));
    legacy.setValue(QStringLiteral("Outlook Integration/BackupInBoxFolder"), QStringLiteral("Mail/Received"));
    legacy.setValue(QStringLiteral("Outlook Integration/BackupSentFolder"), QStringLiteral("Mail/Sent"));
    legacy.sync();

    FakeServices services;
    services.capabilityList.append(QStringLiteral("process.launch"));
    services.replies.insert(QStringLiteral("ui.openUrl"), true);
    services.replies.insert(QStringLiteral("app.defaults"),
                            QVariantMap{{QStringLiteral("managingCompanyName"), QStringLiteral("Acme Corp")},
                                        {QStringLiteral("projectManagerId"), QStringLiteral("person-jim")},
                                        {QStringLiteral("projectManagerName"), QStringLiteral("Jim Smith")}});
    services.replies.insert(QStringLiteral("ui.showDialog"),
                            QVariantMap{{QStringLiteral("name"), QStringLiteral("Quinn Quick")},
                                        {QStringLiteral("email"), QStringLiteral("quinn@initech.example")},
                                        {QStringLiteral("newCompany"), QStringLiteral("Initech")},
                                        {QStringLiteral("role"), QStringLiteral("Controls")}});

    // Microsoft Graph: one To Do list, no tasks yet, one inbox message about P4000.
    const QString projectFolder = m_dir.filePath(QStringLiteral("P4000 folder"));
    QDir().mkpath(projectFolder);
    QVariantList graphCalls;
    services.handler = [&](const QString& method, const QVariantList& args, const Reply& reply) {
        auto ok = [&](int status, const QVariantMap& body) {
            reply(QVariantMap{{QStringLiteral("status"), status}, {QStringLiteral("body"), body}}, {});
        };
        if (method == QLatin1String("app.projectFolder")) {
            reply(projectFolder, {});
            return true;
        }
        if (method == QLatin1String("office365.status")) {
            reply(QVariantMap{{QStringLiteral("signedIn"), true}}, {});
            return true;
        }
        if (method != QLatin1String("office365.graph"))
            return false;
        const QString verb = args.value(0).toString();
        const QString path = args.value(1).toString();
        graphCalls.append(QVariant(QVariantList{verb, path, args.value(2)}));
        if (verb == QLatin1String("GET") && path == QLatin1String("me/mailFolders/inbox/messages"))
            ok(200, {{QStringLiteral("value"), QVariantList{QVariantMap{
                {QStringLiteral("id"), QStringLiteral("M1")}, {QStringLiteral("subject"), QStringLiteral("P4000 rack update")},
                {QStringLiteral("sentDateTime"), QStringLiteral("2026-09-12T14:05:00Z")}}}}});
        else if (verb == QLatin1String("GET") && path == QLatin1String("me/messages/M1"))
            ok(200, {{QStringLiteral("id"), QStringLiteral("M1")}, {QStringLiteral("subject"), QStringLiteral("P4000 rack update")},
                     {QStringLiteral("sentDateTime"), QStringLiteral("2026-09-12T14:05:00Z")},
                     {QStringLiteral("body"), QVariantMap{{QStringLiteral("content"), QStringLiteral("<p>Shipped.</p>")}}}});
        else
            ok(200, {{QStringLiteral("value"), QVariantList{}}});
        return true;
    };

    PluginRegistry registry(&services);
    registry.setPaths({QStringLiteral(PN_REPO_PLUGINS), m_dir.filePath(QStringLiteral("bundled-user")),
                       m_dir.filePath(QStringLiteral("bundled-data")), m_dir.filePath(QStringLiteral("bundled-temp"))});
    registry.setDatabasePath(database);
    registry.setSettingsOrganization(organization);
    registry.setAppVersion(QStringLiteral("6.5.0"));
    registry.setHotReload(false);
    QSignalSpy errors(&registry, &PluginRegistry::pluginError);
    registry.loadAll();

    const QStringList expected{QStringLiteral("com.projectnotespro.base"), QStringLiteral("com.projectnotespro.myshortcuts"),
                               QStringLiteral("com.projectnotespro.team-member-quick-add"),
                               QStringLiteral("com.projectnotespro.archive-mail"),
                               QStringLiteral("com.projectnotespro.office-tools")};
    for (const QString& id : expected) {
        const PluginInfo info = registry.plugin(id);
        QVERIFY2(info.valid, qPrintable(id + QStringLiteral(": ") + info.loadError));
        QVERIFY2(info.enabled, qPrintable(id));
        if (!info.platformSupported)
            continue;
        QTRY_VERIFY2_WITH_TIMEOUT(registry.isRunning(id), qPrintable(id + QStringLiteral(": ") + registry.plugin(id).lastError), 10000);
    }
    QTest::qWait(200);   // let onStart work finish
    QVERIFY2(errors.isEmpty(), qPrintable(errors.isEmpty() ? QString() : errors.first().at(0).toString() + QStringLiteral(": ")
                                                                          + errors.first().at(1).toString()));

    // Base: copy an email address to the clipboard.
    const QString base = QStringLiteral("com.projectnotespro.base");
    registry.runMenu(base, QStringLiteral("copy-contact-email"), QStringLiteral("contacts"), QStringLiteral("person-mike"));
    QTRY_COMPARE(services.argsOf(base, QStringLiteral("ui.clipboardSetText")).size(), 1);
    QCOMPARE(services.argsOf(base, QStringLiteral("ui.clipboardSetText")).first().value(0).toString(),
             QStringLiteral("mike@globex.example"));
    registry.runMenu(base, QStringLiteral("copy-team-email"), QStringLiteral("project_team"), QStringLiteral("team-jim"));
    QTRY_COMPARE(services.argsOf(base, QStringLiteral("ui.clipboardSetText")).size(), 2);
    QCOMPARE(services.argsOf(base, QStringLiteral("ui.clipboardSetText")).last().value(0).toString(),
             QStringLiteral("jim@acme.example"));
    registry.runMenu(base, QStringLiteral("copy-attendee-email"), QStringLiteral("meeting_attendees"), QStringLiteral("att-ann"));
    QTRY_COMPARE(services.argsOf(base, QStringLiteral("ui.clipboardSetText")).size(), 3);
    QCOMPARE(services.argsOf(base, QStringLiteral("ui.clipboardSetText")).last().value(0).toString(),
             QStringLiteral("ann@acme.example"));
    bool editorMenu = false;
    for (const QVariant& menu : registry.globalMenus())
        editorMenu |= menu.toMap().value(QStringLiteral("pluginId")).toString() == base;
    QVERIFY(editorMenu);
    registry.runMenu(base, QStringLiteral("script-editor"), {}, {});
    QTRY_COMPARE(services.argsOf(base, QStringLiteral("process.launch")).size(), 1);
    QVERIFY2(registry.plugin(base).lastError.isEmpty(), qPrintable(registry.plugin(base).lastError));

    // My Shortcuts: the legacy shortcut was migrated to contacts and opens
    // with the record's values.
    const QString shortcuts = QStringLiteral("com.projectnotespro.myshortcuts");
    QString shortcutId;
    for (const QVariant& menu : registry.menusForEntity(QStringLiteral("contacts")))
        if (menu.toMap().value(QStringLiteral("pluginId")).toString() == shortcuts)
            shortcutId = menu.toMap().value(QStringLiteral("menuId")).toString();
    QVERIFY2(!shortcutId.isEmpty(), qPrintable(QJsonDocument::fromVariant(registry.menusForEntity(QStringLiteral("contacts"))).toJson()));
    registry.runMenu(shortcuts, shortcutId, QStringLiteral("contacts"), QStringLiteral("person-mike"));
    QTRY_COMPARE(services.argsOf(shortcuts, QStringLiteral("ui.openUrl")).size(), 1);
    QCOMPARE(services.argsOf(shortcuts, QStringLiteral("ui.openUrl")).first().value(0).toString(),
             QStringLiteral("https://example.test/?q=Mike Smith"));

    // Team Member Quick Add: the dialog's answers become a company, a contact
    // and a team row.
    const QString quickAdd = QStringLiteral("com.projectnotespro.team-member-quick-add");
    registry.runMenu(quickAdd, QStringLiteral("quick-add"), QStringLiteral("project_team"), QStringLiteral("team-jim"));
    QTRY_COMPARE_WITH_TIMEOUT(services.argsOf(quickAdd, QStringLiteral("ui.notify")).size(), 1, 10000);
    QVERIFY2(registry.plugin(quickAdd).lastError.isEmpty(), qPrintable(registry.plugin(quickAdd).lastError));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("bundled-check"));
        db.setDatabaseName(database);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY2(q.exec(QStringLiteral("SELECT c.client_name, pp.role FROM project_people pp JOIN people p ON p.id = pp.people_id "
                                      "JOIN clients c ON c.id = p.client_id WHERE p.name = 'Quinn Quick'")), qPrintable(q.lastError().text()));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("Initech"));
        QCOMPARE(q.value(1).toString(), QStringLiteral("Controls"));
        q.finish();
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("bundled-check"));

    // Archive Mail over Graph: the project's email lands in its folder as .eml.
    const QString outlook = QStringLiteral("com.projectnotespro.archive-mail");
    registry.runMenu(outlook, QStringLiteral("download-graph"), QStringLiteral("projects"), QStringLiteral("project-p4000"));
    QTRY_COMPARE_WITH_TIMEOUT(services.argsOf(outlook, QStringLiteral("ui.notify")).size(), 1, 10000);
    QVERIFY2(registry.plugin(outlook).lastError.isEmpty(), qPrintable(registry.plugin(outlook).lastError));
    const QString eml = projectFolder + QStringLiteral("/Mail/Received/20260912T140500Z-P4000 rack update.eml");
    QVERIFY2(QFileInfo::exists(eml), qPrintable(QDir(projectFolder + QStringLiteral("/Mail/Received")).entryList().join(QLatin1Char(','))));
    QFile emlFile(eml);
    QVERIFY(emlFile.open(QIODevice::ReadOnly));
    QVERIFY(emlFile.readAll().contains("<p>Shipped.</p>"));
    QVERIFY(QFileInfo::exists(projectFolder + QStringLiteral("/Mail/Received/download_state.json")));

    // Office Tools is Windows-only. A copy without the platform limit checks
    // the parts that do not need Office: a template copied into the project
    // folder and recorded as a location, and clear errors where COM is missing.
    const QString officeRoot = m_dir.filePath(QStringLiteral("office-root"));
    const QString officeCopy = officeRoot + QStringLiteral("/office-tools");
    QDir().mkpath(officeCopy + QStringLiteral("/templates"));
    QFile manifestFile(QStringLiteral(PN_REPO_PLUGINS) + QStringLiteral("/office-tools/plugin.json"));
    QVERIFY(manifestFile.open(QIODevice::ReadOnly));
    QJsonObject officeManifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
    officeManifest.remove(QStringLiteral("platforms"));
    writeFile(officeCopy + QStringLiteral("/plugin.json"), QJsonDocument(officeManifest).toJson());
    for (const QString& name : {QStringLiteral("main.mjs"), QStringLiteral("settings.schema.json"),
                                QStringLiteral("templates/Risk Register Template.xlsx")})
        QVERIFY(QFile::copy(QStringLiteral(PN_REPO_PLUGINS) + QStringLiteral("/office-tools/") + name, officeCopy + QLatin1Char('/') + name));
    services.replies.insert(QStringLiteral("ui.pickFile"), officeCopy + QStringLiteral("/templates/Risk Register Template.xlsx"));
    services.replies.insert(QStringLiteral("ui.choose"), QVariant::fromValue(nullptr));

    PluginRegistry officeRegistry(&services);
    officeRegistry.setPaths({officeRoot, m_dir.filePath(QStringLiteral("office-user")),
                             m_dir.filePath(QStringLiteral("office-data")), m_dir.filePath(QStringLiteral("office-temp"))});
    officeRegistry.setDatabasePath(database);
    officeRegistry.setSettingsOrganization(organization);
    officeRegistry.setAppVersion(QStringLiteral("6.5.0"));
    officeRegistry.setHotReload(false);
    QSignalSpy officeErrors(&officeRegistry, &PluginRegistry::pluginError);
    officeRegistry.loadAll();
    const QString office = QStringLiteral("com.projectnotespro.office-tools");
    QTRY_VERIFY2_WITH_TIMEOUT(officeRegistry.isRunning(office), qPrintable(officeRegistry.plugin(office).lastError), 10000);

    officeRegistry.runMenu(office, QStringLiteral("pcr-register"), QStringLiteral("projects"), QStringLiteral("project-p4000"));
    QTRY_COMPARE_WITH_TIMEOUT(services.argsOf(office, QStringLiteral("ui.openUrl")).size(), 1, 10000);
    const QString copied = projectFolder + QStringLiteral("/Project Management/PCR's/P4000 Risk Register.xlsx");
    QVERIFY2(QFileInfo::exists(copied), qPrintable(officeRegistry.plugin(office).lastError));
    QTRY_VERIFY2(officeRegistry.plugin(office).lastError.isEmpty() && officeErrors.isEmpty(),
                 qPrintable(officeRegistry.plugin(office).lastError));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("office-check"));
        db.setDatabaseName(database);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QTRY_VERIFY(q.exec(QStringLiteral("SELECT location_type, full_path FROM project_locations "
                                          "WHERE location_description = 'P4000 Risk Register.xlsx'")) && q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("Excel Document"));
        QCOMPARE(q.value(1).toString(), QString(copied).replace(QLatin1Char('/'), QLatin1Char('\\')));
        q.finish();
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("office-check"));

    // Without COM the Office commands report the problem instead of failing silently.
    const int officeMessages = services.messageCount(office);
    officeRegistry.runMenu(office, QStringLiteral("kill-excel"), {}, {});
    QTRY_VERIFY_WITH_TIMEOUT(services.messageCount(office) > officeMessages, 10000);
    QVERIFY(officeErrors.isEmpty());

    QVERIFY2(errors.isEmpty(), qPrintable(errors.isEmpty() ? QString() : errors.first().at(1).toString()));
}

// The separately shipped IFS Cloud plugin against a fake IFS server: project
// import, tracker item sync in both directions, and the SSRS status report.
void PluginHostTest::ifsCloudPlugin()
{
    const QString database = m_dir.filePath(QStringLiteral("ifs.db"));
    QVERIFY(QFile::copy(m_pristine, database));
    const QString organization = QStringLiteral("ProjectNotesIfsTest");
    QSettings(organization, QStringLiteral("PluginState")).clear();

    auto json = [](const QVariant& v) { return QJsonDocument::fromVariant(v).toJson(QJsonDocument::Compact); };
    auto values = [&json](const QVariantList& list) { return json(QVariantMap{{QStringLiteral("value"), list}}); };
    FakeHttpServer ifs([&](const FakeHttpServer::Request& r) -> QPair<int, QByteArray> {
        const QString t = r.target;
        if (t.contains(QLatin1String("/.well-known/openid-configuration")))
            return {200, json(QVariantMap{{QStringLiteral("authorization_endpoint"), QStringLiteral("http://127.0.0.1/auth")},
                                          {QStringLiteral("token_endpoint"), QStringLiteral("http://127.0.0.1/token")}})};
        if (t.contains(QLatin1String("Reference_FndUser")))
            return {200, values({QVariantMap{{QStringLiteral("Identity"), QStringLiteral("PMUSER")}}})};
        if (t.contains(QLatin1String("ResourceSet")))
            return {200, values({QVariantMap{{QStringLiteral("ResourceId"), QStringLiteral("ENG")}, {QStringLiteral("Description"), QStringLiteral("Engineer")}}})};
        if (t.contains(QLatin1String("ProjectsHandling.svc/Projects")) && t.contains(QLatin1String("Manager eq 'PMUSER'")))
            return {200, values({
                QVariantMap{{QStringLiteral("ProjectId"), QStringLiteral("P4000")}, {QStringLiteral("Objstate"), QStringLiteral("Started")},
                            {QStringLiteral("Description"), QStringLiteral("IT New Server Install")}, {QStringLiteral("CompanyName"), QStringLiteral("Acme Corp")},
                            {QStringLiteral("CustomerIdRef"), QVariantMap{{QStringLiteral("Name"), QStringLiteral("Acme Corp")}}}},
                QVariantMap{{QStringLiteral("ProjectId"), QStringLiteral("P5000")}, {QStringLiteral("Objstate"), QStringLiteral("Approved")},
                            {QStringLiteral("Description"), QStringLiteral("Packaging Line")}, {QStringLiteral("CompanyName"), QStringLiteral("Acme Corp")},
                            {QStringLiteral("CustomerIdRef"), QVariantMap{{QStringLiteral("Name"), QStringLiteral("Initech")}}}}})};
        if (t.contains(QLatin1String("ProjectsHandling.svc/Projects")))
            return {200, values({QVariantMap{{QStringLiteral("ProjectId"), QStringLiteral("x")}, {QStringLiteral("Objstate"), QStringLiteral("Started")}}})};
        if (t.contains(QLatin1String("ProjectAnalysisArray")))
            return {200, values({QVariantMap{{QStringLiteral("Bac_aggr_"), 5000}}})};
        if (t.contains(QLatin1String("ProjectCostArray")))
            return {200, values({QVariantMap{{QStringLiteral("Baseline_aggr_"), 4000}, {QStringLiteral("Used_aggr_"), 1000},
                                             {QStringLiteral("EarnedValue_aggr_"), 900}, {QStringLiteral("ScheduledWork_aggr_"), 1200}}})};
        if (t.contains(QLatin1String("ProjResourceAllocations")))
            return {200, values({QVariantMap{{QStringLiteral("ResourceParentId"), QStringLiteral("ENG")},
                                             {QStringLiteral("EmployeeIdRef"), QVariantMap{{QStringLiteral("EmployeeName"), QStringLiteral("Pat Engineer")}}}}})};
        if (t.contains(QLatin1String("ActivityListHandling.svc/Activities")))
            return {200, t.contains(QLatin1String("'P4000'")) ? values({QVariantMap{{QStringLiteral("ActivitySeq"), 77}}}) : values({})};
        if (r.method == "GET" && t.contains(QLatin1String("Activities(ActivitySeq=77)/ActivityTasks")))
            return {200, values({QVariantMap{{QStringLiteral("TaskId"), QStringLiteral("T100")}, {QStringLiteral("Name"), QStringLiteral("Fix panel")},
                                             {QStringLiteral("Info"), QStringLiteral("Reported in IFS")}, {QStringLiteral("Cf_Status"), QStringLiteral("CfEnum_NEW")},
                                             {QStringLiteral("Cf_Priority"), QStringLiteral("CfEnum_HIGH")},
                                             {QStringLiteral("Cf_Date_Updated"), QStringLiteral("2026-09-12T10:00:00Z")},
                                             {QStringLiteral("Cf_Assigned_To"), QStringLiteral("Jim Smith")}}})};
        if (r.method == "POST" && t.endsWith(QLatin1String("ActivityTasks")))
            return {201, r.body};
        if (t.startsWith(QLatin1String("/ReportServer/")))
            return {200, t.contains(QLatin1String("Format=PDF")) ? QByteArray("%PDF-1.4 fake") : QByteArray("PK fake docx")};
        return {404, "{}"};
    });

    // A copy whose manifest allows the fake server, with the settings the
    // Python plugin left behind.
    const QString root = m_dir.filePath(QStringLiteral("ifs-root"));
    const QString copy = root + QStringLiteral("/ifs-cloud");
    QDir().mkpath(copy);
    for (const QString& name : QDir(QStringLiteral(PN_REPO_ROOT) + QStringLiteral("/extras/ifs-cloud")).entryList(QDir::Files))
        QVERIFY(QFile::copy(QStringLiteral(PN_REPO_ROOT) + QStringLiteral("/extras/ifs-cloud/") + name, copy + QLatin1Char('/') + name));
    QFile manifestFile(copy + QStringLiteral("/plugin.json"));
    QVERIFY(manifestFile.open(QIODevice::ReadOnly));
    QJsonObject ifsManifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
    manifestFile.close();
    QVERIFY(!ifsManifest.isEmpty());
    ifsManifest.insert(QStringLiteral("http"), QJsonArray{QStringLiteral("127.0.0.1")});
    QFile::remove(copy + QStringLiteral("/plugin.json"));
    writeFile(copy + QStringLiteral("/plugin.json"), QJsonDocument(ifsManifest).toJson());

    QSettings legacy(organization, QStringLiteral("PluginSettings"));
    legacy.clear();
    legacy.setValue(QStringLiteral("IFS Cloud/URL"), ifs.base());
    legacy.setValue(QStringLiteral("IFS Cloud/SyncTrackerItems"), QStringLiteral("true"));
    legacy.setValue(QStringLiteral("IFS Cloud/ReportServer"), QStringLiteral("127.0.0.1:%1").arg(ifs.port()));
    legacy.setValue(QStringLiteral("IFS Cloud/DomainUser"), QStringLiteral("CORP\\pm"));
    legacy.setValue(QStringLiteral("IFS Cloud/DomainPassword"), QStringLiteral("hunter2"));
    legacy.sync();

    // A signed-in token, so no browser is needed.
    const QByteArray claims = QByteArray(R"({"preferred_username":"pm@example.test"})").toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    const QString token = QString::fromUtf8(json(QVariantMap{{QStringLiteral("access_token"), QStringLiteral("h.") + QString::fromLatin1(claims) + QStringLiteral(".s")},
                                                             {QStringLiteral("expires_at"), QDateTime::currentSecsSinceEpoch() + 3600}}));
    FakeServices services;
    QHash<QString, QString> secretStore{{QStringLiteral("sso-token"), token}};
    const QString projectFolder = m_dir.filePath(QStringLiteral("ifs-project"));
    QDir().mkpath(projectFolder);
    services.handler = [&](const QString& method, const QVariantList& args, const Reply& reply) {
        if (method == QLatin1String("secrets.get")) { reply(secretStore.value(args.value(0).toString()), {}); return true; }
        if (method == QLatin1String("secrets.set")) { secretStore.insert(args.value(0).toString(), args.value(1).toString()); reply({}, {}); return true; }
        if (method == QLatin1String("app.projectFolder")) { reply(projectFolder, {}); return true; }
        if (method == QLatin1String("ui.showForm")) {
            reply(QVariantMap{{QStringLiteral("reportDate"), QStringLiteral("2026-10-01")}, {QStringLiteral("delivery"), QStringLiteral("Email the PDF")},
                              {QStringLiteral("keepWord"), false}, {QStringLiteral("display"), false}}, {});
            return true;
        }
        if (method == QLatin1String("email.audiences")) { reply(QVariantList{}, {}); return true; }
        if (method == QLatin1String("email.compose")) { reply(QVariantMap{{QStringLiteral("status"), QStringLiteral("handed-off")}}, {}); return true; }
        if (method == QLatin1String("app.defaults")) { reply(QVariantMap{{QStringLiteral("projectManagerId"), QStringLiteral("person-mike")}}, {}); return true; }
        return false;
    };

    PluginRegistry registry(&services);
    registry.setPaths({root, m_dir.filePath(QStringLiteral("ifs-user")), m_dir.filePath(QStringLiteral("ifs-data")), m_dir.filePath(QStringLiteral("ifs-temp"))});
    registry.setDatabasePath(database);
    registry.setSettingsOrganization(organization);
    registry.setAppVersion(QStringLiteral("6.5.0"));
    registry.setHotReload(false);
    QSignalSpy errors(&registry, &PluginRegistry::pluginError);
    registry.loadAll();
    const QString id = QStringLiteral("com.projectnotespro.ifs-cloud");
    QTRY_VERIFY2_WITH_TIMEOUT(registry.isRunning(id), qPrintable(registry.plugin(id).loadError + registry.plugin(id).lastError), 10000);

    // The password moved to the credential store; shortcut menus appeared.
    QTRY_COMPARE(secretStore.value(QStringLiteral("DomainPassword")), QStringLiteral("hunter2"));
    QVERIFY(!QSettings(organization, QStringLiteral("PluginSettings")).contains(id + QStringLiteral("/DomainPassword")));
    auto shortcutMenus = [&registry, &id] {
        int n = 0;
        for (const QVariant& menu : registry.menusForEntity(QStringLiteral("projects")))
            n += menu.toMap().value(QStringLiteral("pluginId")).toString() == id;
        return n;
    };
    QTRY_COMPARE(shortcutMenus(), 6 + 7 + 1);   // IFS pages (one more is on contacts), reports, Generate Status Report

    registry.runMenu(id, QStringLiteral("import"), {}, {});
    QTRY_COMPARE_WITH_TIMEOUT(services.argsOf(id, QStringLiteral("ui.notify")).size(), 1, 15000);
    QVERIFY2(errors.isEmpty(), qPrintable(errors.isEmpty() ? QString() : errors.first().at(1).toString()));
    QVERIFY2(services.argsOf(id, QStringLiteral("ui.notify")).first().value(0).toString().contains(QStringLiteral("Imported 2")),
             qPrintable(services.argsOf(id, QStringLiteral("ui.notify")).first().value(0).toString()));

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("ifs-check"));
        db.setDatabaseName(database);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("SELECT c.client_name, p.budget FROM projects p JOIN clients c ON c.id = p.client_id WHERE p.project_number = 'P5000'")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("Initech"));
        QCOMPARE(q.value(1).toDouble(), 4000.0);
        QVERIFY(q.exec(QStringLiteral("SELECT pp.role FROM project_people pp JOIN people p ON p.id = pp.people_id "
                                      "JOIN projects pr ON pr.id = pp.project_id WHERE p.name = 'Pat Engineer' AND pr.project_number = 'P5000'")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("Engineer"));
        QVERIFY(q.exec(QStringLiteral("SELECT i.item_name, i.priority, pe.name FROM item_tracker i JOIN people pe ON pe.id = i.assigned_to "
                                      "WHERE i.item_number = 'IFST100'")));
        QVERIFY2(q.next(), "the IFS task did not become a tracker item");
        QCOMPARE(q.value(0).toString(), QStringLiteral("Fix panel"));
        QCOMPARE(q.value(1).toString(), QStringLiteral("High"));
        QCOMPARE(q.value(2).toString(), QStringLiteral("Jim Smith"));
        q.finish();
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("ifs-check"));

    // Item 1 exists only in Project Notes, so it was created in IFS; item 2
    // is internal and stays out.
    QStringList created;
    for (const FakeHttpServer::Request& r : std::as_const(ifs.requests))
        if (r.method == "POST")
            created << QJsonDocument::fromJson(r.body).object().value(QStringLiteral("TaskId")).toString();
    QCOMPARE(created, QStringList{QStringLiteral("P40001")});
    for (const FakeHttpServer::Request& r : std::as_const(ifs.requests))
        if (!r.target.contains(QLatin1String("openid-configuration")))
            QVERIFY2(r.authorization.startsWith("Bearer "), qPrintable(r.target));

    // SSRS status report: downloaded with the domain account and emailed as PDF.
    const int requestsBefore = ifs.requests.size();
    const int callsBefore = services.calls.size();
    registry.runMenu(id, QStringLiteral("status-report"), QStringLiteral("projects"), QStringLiteral("project-p4000"));
    QElapsedTimer waited;
    waited.start();
    while (services.argsOf(id, QStringLiteral("ui.notify")).size() < 2 && waited.elapsed() < 15000)
        QTest::qWait(20);
    if (services.argsOf(id, QStringLiteral("ui.notify")).size() < 2) {
        for (int i = requestsBefore; i < ifs.requests.size(); ++i)
            qWarning() << "request" << ifs.requests.at(i).method << ifs.requests.at(i).target;
        for (int i = callsBefore; i < services.calls.size(); ++i)
            qWarning() << "call" << services.calls.at(i).method;
    }
    QCOMPARE(services.argsOf(id, QStringLiteral("ui.notify")).size(), 2);
    QVERIFY2(registry.plugin(id).lastError.isEmpty(), qPrintable(registry.plugin(id).lastError));
    const QList<QVariantList> composed = services.argsOf(id, QStringLiteral("email.compose"));
    QCOMPARE(composed.size(), 1);
    const QVariantMap message = composed.first().value(0).toMap();
    QCOMPARE(message.value(QStringLiteral("subject")).toString(), QStringLiteral("P4000 IT New Server Install Status Report 10/01/2026"));
    const QString pdf = message.value(QStringLiteral("attachments")).toList().value(0).toString();
    QFile pdfFile(pdf);
    QVERIFY2(pdfFile.open(QIODevice::ReadOnly), qPrintable(pdf));
    QCOMPARE(pdfFile.readAll(), QByteArray("%PDF-1.4 fake"));
    // Jim receives status reports; Mike is the project manager.
    QCOMPARE(message.value(QStringLiteral("to")).toList().size(), 1);
    QCOMPARE(message.value(QStringLiteral("to")).toList().first().toMap().value(QStringLiteral("email")).toString(), QStringLiteral("jim@acme.example"));
    bool basicAuth = false;
    for (const FakeHttpServer::Request& r : std::as_const(ifs.requests))
        if (r.target.startsWith(QLatin1String("/ReportServer/")))
            basicAuth = r.authorization == "Basic " + QByteArray("CORP\\pm:hunter2").toBase64();
    QVERIFY(basicAuth);
    QVERIFY(errors.isEmpty());
}

// A fresh install, with no settings from the Python plugin, still gets every
// shortcut menu: the IFS address and report server start filled in.
void PluginHostTest::ifsCloudDefaultShortcuts()
{
    const QString database = m_dir.filePath(QStringLiteral("ifs-defaults.db"));
    QVERIFY(QFile::copy(m_pristine, database));
    const QString organization = QStringLiteral("ProjectNotesIfsDefaultsTest");
    QSettings(organization, QStringLiteral("PluginState")).clear();
    QSettings(organization, QStringLiteral("PluginSettings")).clear();

    const QString root = m_dir.filePath(QStringLiteral("ifs-defaults-root"));
    const QString copy = root + QStringLiteral("/ifs-cloud");
    QDir().mkpath(copy);
    for (const QString& name : QDir(QStringLiteral(PN_REPO_ROOT) + QStringLiteral("/extras/ifs-cloud")).entryList(QDir::Files))
        QVERIFY(QFile::copy(QStringLiteral(PN_REPO_ROOT) + QStringLiteral("/extras/ifs-cloud/") + name, copy + QLatin1Char('/') + name));

    FakeServices services;
    PluginRegistry registry(&services);
    registry.setPaths({root, m_dir.filePath(QStringLiteral("ifs-defaults-user")), m_dir.filePath(QStringLiteral("ifs-defaults-data")),
                       m_dir.filePath(QStringLiteral("ifs-defaults-temp"))});
    registry.setDatabasePath(database);
    registry.setSettingsOrganization(organization);
    registry.setAppVersion(QStringLiteral("6.5.0"));
    registry.setHotReload(false);
    registry.loadAll();
    const QString id = QStringLiteral("com.projectnotespro.ifs-cloud");
    QTRY_VERIFY2_WITH_TIMEOUT(registry.isRunning(id), qPrintable(registry.plugin(id).loadError + registry.plugin(id).lastError), 10000);

    auto shortcutMenus = [&registry, &id](const QString& entity) {
        int n = 0;
        for (const QVariant& menu : registry.menusForEntity(entity))
            n += menu.toMap().value(QStringLiteral("pluginId")).toString() == id;
        return n;
    };
    QTRY_COMPARE(shortcutMenus(QStringLiteral("projects")), 6 + 7 + 1);   // IFS pages, reports, Generate Status Report
    QCOMPARE(shortcutMenus(QStringLiteral("contacts")), 1);                // Open Time Entry

    QSettings stored(organization, QStringLiteral("PluginSettings"));
    QCOMPARE(stored.value(id + QStringLiteral("/URL")).toString(), QStringLiteral("https://ifs.cornerstonecontrols.com"));
    QCOMPARE(stored.value(id + QStringLiteral("/ReportServer")).toString(), QStringLiteral("indvifsbi05"));
}

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    PluginHostTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_pluginhost.moc"

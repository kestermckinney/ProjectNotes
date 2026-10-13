// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Pins XML export/import output across the RecordTree refactor. Golden files
// under fixtures/recordtree/golden/ were produced by the per-row exporter that
// predates RecordTree; the batched exporter must reproduce them exactly.
//
// Set PN_RECORDTREE_WRITE_GOLDEN=1 to (re)write the golden files from the
// current code instead of comparing.

#include "databaseobjects.h"
#include "recordtree.h"
#include "sqlquerymodel.h"

#include <QCoreApplication>
#include <QDir>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QFile>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

QString fixturePath(const QString &name)
{
    return QStringLiteral(PN_RECORDTREE_FIXTURES) + QLatin1Char('/') + name;
}

QDomDocument loadXml(const QString &path)
{
    QFile file(path);
    QDomDocument document;
    if (file.open(QIODevice::ReadOnly))
        document.setContent(file.readAll());
    return document;
}

// The root carries the database path and export time; drop both so exports
// from different databases and runs compare equal.
QString normalized(QDomDocument *document)
{
    QDomElement root = document->documentElement();
    root.removeAttribute(QStringLiteral("filepath"));
    root.removeAttribute(QStringLiteral("export_date"));
    return document->toString(1);
}

QString exportTable(DatabaseObjects &dbo, const QString &table, const QString &key, const QString &filter)
{
    SqlQueryModel *model = dbo.createExportObject(table);
    if (!model)
        return {};
    if (!key.isEmpty())
        model->setFilter(0, key);
    model->refresh();
    QDomDocument *document = dbo.createXMLExportDoc(model, filter);
    const QString text = normalized(document);
    delete document;
    delete model;
    return text;
}

} // namespace

class RecordTreeTest final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void exportsMatchGolden_data();
    void exportsMatchGolden();
    void multiModelExportMatchesGolden();
    void importRoundTripReproducesExport();
    void importIsIdempotent();
    void deleteRowCascadesToOwnedChildren();
    void batchedExportQueryCount();
    void batchCommitAndRollback();

private:
    // DatabaseStructure creates the schema through global_DBObjects, so an
    // empty database is created once that way and copied for each test db.
    QString freshDatabase(const QString &name);
    QTemporaryDir m_dir;
    QString m_emptyDatabase;
    DatabaseObjects m_dbo {nullptr};
};

QString RecordTreeTest::freshDatabase(const QString &name)
{
    const QString path = m_dir.filePath(name);
    QFile::remove(path);
    return QFile::copy(m_emptyDatabase, path) ? path : QString();
}

void RecordTreeTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_emptyDatabase = m_dir.filePath(QStringLiteral("empty.db"));
    QVERIFY(global_DBObjects.createDatabase(m_emptyDatabase));
    const QString path = freshDatabase(QStringLiteral("recordtree.db"));
    QVERIFY(!path.isEmpty());
    QVERIFY(m_dbo.openDatabase(path, QStringLiteral("recordtree-test"), false));
    QVERIFY(m_dbo.importXMLDoc(loadXml(fixturePath(QStringLiteral("import.xml")))));
    if (qEnvironmentVariableIsSet("PN_RECORDTREE_WRITE_GOLDEN"))
        QDir().mkpath(fixturePath(QStringLiteral("golden")));
}

void RecordTreeTest::cleanupTestCase()
{
    m_dbo.closeDatabase();
}

void RecordTreeTest::exportsMatchGolden_data()
{
    QTest::addColumn<QString>("table");
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("filter");

    QTest::newRow("clients") << "clients" << "" << "";
    QTest::newRow("people") << "people" << "" << "";
    QTest::newRow("people-filtered") << "people" << "person-jim" << "people";
    QTest::newRow("projects") << "projects" << "" << "";
    QTest::newRow("project-p4000") << "projects" << "project-p4000" << "";
    QTest::newRow("project-locations-filter") << "projects" << "project-p4000" << "projects/project_locations";
    QTest::newRow("project-only-filter") << "projects" << "project-p4000" << "projects";
    QTest::newRow("project-excluded") << "projects" << "project-p4000" << "people";
    QTest::newRow("project_people") << "project_people" << "" << "";
    QTest::newRow("project_locations") << "project_locations" << "" << "";
    QTest::newRow("status_report_items") << "status_report_items" << "" << "";
    QTest::newRow("project_email_audiences") << "project_email_audiences" << "" << "";
    QTest::newRow("project_notes") << "project_notes" << "" << "";
    QTest::newRow("note-kickoff") << "project_notes" << "note-kickoff" << "";
    QTest::newRow("item_tracker") << "item_tracker" << "" << "";
    QTest::newRow("item-1-tracker-filter") << "item_tracker" << "item-1" << "item_tracker";
    QTest::newRow("meeting_attendees") << "meeting_attendees" << "" << "";
    QTest::newRow("item_tracker_updates") << "item_tracker_updates" << "" << "";
}

void RecordTreeTest::exportsMatchGolden()
{
    QFETCH(QString, table);
    QFETCH(QString, key);
    QFETCH(QString, filter);

    const QString actual = exportTable(m_dbo, table, key, filter);
    QVERIFY(!actual.isEmpty());
    const QString golden = fixturePath(QStringLiteral("golden/%1.xml").arg(QString::fromLatin1(QTest::currentDataTag())));

    if (qEnvironmentVariableIsSet("PN_RECORDTREE_WRITE_GOLDEN")) {
        QFile file(golden);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(actual.toUtf8());
        return;
    }
    QFile file(golden);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(golden));
    QCOMPARE(actual, QString::fromUtf8(file.readAll()));
}

void RecordTreeTest::multiModelExportMatchesGolden()
{
    QList<SqlQueryModel *> *models = new QList<SqlQueryModel *>;
    for (const QString &table : {QStringLiteral("clients"), QStringLiteral("people"), QStringLiteral("projects")}) {
        SqlQueryModel *model = m_dbo.createExportObject(table);
        model->refresh();
        models->append(model);
    }
    QDomDocument *document = m_dbo.createXMLExportDoc(models);
    const QString actual = normalized(document);
    delete document;
    qDeleteAll(*models);
    delete models;

    const QString golden = fixturePath(QStringLiteral("golden/all.xml"));
    if (qEnvironmentVariableIsSet("PN_RECORDTREE_WRITE_GOLDEN")) {
        QFile file(golden);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(actual.toUtf8());
        return;
    }
    QFile file(golden);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(actual, QString::fromUtf8(file.readAll()));
}

void RecordTreeTest::importRoundTripReproducesExport()
{
    if (qEnvironmentVariableIsSet("PN_RECORDTREE_WRITE_GOLDEN"))
        QSKIP("writing golden files");

    // Import the full export into an empty database and export it again.
    const QString path = freshDatabase(QStringLiteral("roundtrip.db"));
    DatabaseObjects second(nullptr);
    QVERIFY(second.openDatabase(path, QStringLiteral("recordtree-roundtrip"), false));
    QVERIFY(second.importXMLDoc(loadXml(fixturePath(QStringLiteral("golden/all.xml")))));

    for (const QString &table : {QStringLiteral("clients"), QStringLiteral("people"), QStringLiteral("projects")}) {
        QFile file(fixturePath(QStringLiteral("golden/%1.xml").arg(table)));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QString actual = exportTable(second, table, {}, {});
        if (qEnvironmentVariableIsSet("PN_RECORDTREE_DUMP")) {
            QFile dump(qEnvironmentVariable("PN_RECORDTREE_DUMP") + QLatin1Char('/') + table + QStringLiteral("-roundtrip.xml"));
            dump.open(QIODevice::WriteOnly);
            dump.write(actual.toUtf8());
            qInfo("%s", qPrintable(dump.fileName()));
        }
        // `comments` is a GROUP_CONCAT of a tracker item's updates, whose order
        // follows insertion order; the round trip inserts them in export order.
        auto withoutComments = [](QString text) {
            static const QRegularExpression comments(QStringLiteral("\\s*<column name=\"comments\">[^<]*</column>"));
            return text.remove(comments);
        };
        QCOMPARE(withoutComments(actual), withoutComments(QString::fromUtf8(file.readAll())));
    }
    second.closeDatabase();
}

void RecordTreeTest::importIsIdempotent()
{
    if (qEnvironmentVariableIsSet("PN_RECORDTREE_WRITE_GOLDEN"))
        QSKIP("writing golden files");

    // Re-importing the same data must not duplicate rows or change values.
    const QString before = exportTable(m_dbo, QStringLiteral("projects"), {}, {});
    QVERIFY(m_dbo.importXMLDoc(loadXml(fixturePath(QStringLiteral("import.xml")))));
    QCOMPARE(exportTable(m_dbo, QStringLiteral("projects"), {}, {}), before);
}

void RecordTreeTest::deleteRowCascadesToOwnedChildren()
{
    if (qEnvironmentVariableIsSet("PN_RECORDTREE_WRITE_GOLDEN"))
        QSKIP("writing golden files");

    // A <row delete="true"> on a tracker item soft-deletes it and its owned
    // comments; a contact still referenced by a project is not deleted.
    const QString xml = QStringLiteral(
        "<projectnotes>"
        "<table name='item_tracker'><row delete='true'><column name='id'>item-1</column></row></table>"
        "<table name='people'><row delete='true'><column name='id'>person-jim</column></row></table>"
        "</projectnotes>");
    QDomDocument document;
    QVERIFY(document.setContent(xml));
    QVERIFY(m_dbo.importXMLDoc(document));

    QCOMPARE(m_dbo.execute(QStringLiteral("select deleted from item_tracker where id='item-1'")), QStringLiteral("1"));
    QCOMPARE(m_dbo.execute(QStringLiteral("select count(*) from item_tracker_updates where item_id='item-1' and deleted=1")), QStringLiteral("2"));
    QCOMPARE(m_dbo.execute(QStringLiteral("select coalesce(deleted,0) from people where id='person-jim'")), QStringLiteral("0"));
}

void RecordTreeTest::batchedExportQueryCount()
{
    if (qEnvironmentVariableIsSet("PN_RECORDTREE_WRITE_GOLDEN"))
        QSKIP("writing golden files");

    // A large project: 300 tracker items with two comments each.
    RecordTree tree;
    RecordTable items{QStringLiteral("item_tracker"), {}, {}};
    RecordTable updates{QStringLiteral("item_tracker_updates"), {}, {}};
    for (int i = 0; i < 300; ++i) {
        const QString id = QStringLiteral("bench-item-%1").arg(i);
        RecordRow item;
        item.columns = {{QStringLiteral("id"), id, {}},
                        {QStringLiteral("item_number"), QString::number(100 + i), {}},
                        {QStringLiteral("item_type"), QStringLiteral("Tracker"), {}},
                        {QStringLiteral("item_name"), QStringLiteral("Bench item %1").arg(i), {}},
                        {QStringLiteral("identified_by"), {}, QStringLiteral("Mike Smith")},
                        {QStringLiteral("assigned_to"), {}, QStringLiteral("Ann O'Neil")},
                        {QStringLiteral("priority"), QStringLiteral("Low"), {}},
                        {QStringLiteral("status"), QStringLiteral("New"), {}},
                        {QStringLiteral("last_update"), QStringLiteral("09/11/2026"), {}},
                        {QStringLiteral("project_id"), {}, QStringLiteral("P4000")}};
        items.rows.append(item);
        for (int u = 0; u < 2; ++u) {
            RecordRow update;
            update.columns = {{QStringLiteral("id"), QStringLiteral("%1-update-%2").arg(id).arg(u), {}},
                              {QStringLiteral("item_id"), id, {}},
                              {QStringLiteral("lastupdated_date"), QStringLiteral("09/1%1/2026").arg(u), {}},
                              {QStringLiteral("update_note"), QStringLiteral("Note %1").arg(u), {}},
                              {QStringLiteral("updated_by"), {}, QStringLiteral("Mike Smith")}};
            updates.rows.append(update);
        }
    }
    tree.tables = {items, updates};
    QList<RecordImportOutcome> outcomes;
    QVERIFY(m_dbo.importRecordTree(tree, &outcomes));
    QCOMPARE(outcomes.size(), 900);
    for (const RecordImportOutcome &outcome : std::as_const(outcomes))
        QCOMPARE(int(outcome.kind), int(RecordImportOutcome::Inserted));

    QScopedPointer<SqlQueryModel> project(m_dbo.createExportObject(QStringLiteral("projects")));
    project->setFilter(0, QStringLiteral("project-p4000"));
    project->refresh();

    RecordTreeExporter exporter(&m_dbo);
    QElapsedTimer timer;
    timer.start();
    const RecordTable table = exporter.exportModel(project.data());
    const qint64 elapsed = timer.elapsed();
    QCOMPARE(table.rows.size(), 1);

    int exportedItems = 0;
    for (const RecordTable &child : table.rows.first().children)
        if (child.name == QLatin1String("item_tracker"))
            exportedItems += int(child.rows.size());
    QVERIFY(exportedItems >= 300);

    // The per-row exporter ran one lookup query per foreign-key cell and one
    // child query per parent row: several thousand statements for this
    // project. The batched exporter's count depends on tree depth only.
    qInfo("batched export: %d queries, %lld ms", exporter.queryCount(), elapsed);
    QVERIFY2(exporter.queryCount() < 60, qPrintable(QString::number(exporter.queryCount())));
}

void RecordTreeTest::batchCommitAndRollback()
{
    if (qEnvironmentVariableIsSet("PN_RECORDTREE_WRITE_GOLDEN"))
        QSKIP("writing golden files");

    auto company = [](const QString &id, const QString &name) {
        RecordTree tree;
        RecordRow row;
        row.columns = {{QStringLiteral("id"), id, {}}, {QStringLiteral("client_name"), name, {}}};
        tree.tables = {RecordTable{QStringLiteral("clients"), {}, {row}}};
        return tree;
    };
    auto exists = [this](const QString &id) {
        return !m_dbo.execute(QStringLiteral("select id from clients where id='%1'").arg(id)).isEmpty();
    };

    QVERIFY(m_dbo.beginBatch());
    QVERIFY(m_dbo.importRecordTree(company(QStringLiteral("batch-a"), QStringLiteral("Batch A"))));
    QVERIFY(m_dbo.importRecordTree(company(QStringLiteral("batch-b"), QStringLiteral("Batch B"))));
    QVERIFY(exists(QStringLiteral("batch-a")));   // visible inside the batch
    m_dbo.rollbackBatch();
    QVERIFY(!exists(QStringLiteral("batch-a")));
    QVERIFY(!exists(QStringLiteral("batch-b")));

    QVERIFY(m_dbo.beginBatch());
    QVERIFY(m_dbo.importRecordTree(company(QStringLiteral("batch-c"), QStringLiteral("Batch C"))));
    QVERIFY(m_dbo.commitBatch());
    QVERIFY(exists(QStringLiteral("batch-c")));

    // A failing statement poisons the batch: commit rolls everything back.
    QVERIFY(m_dbo.beginBatch());
    QVERIFY(m_dbo.importRecordTree(company(QStringLiteral("batch-d"), QStringLiteral("Batch D"))));
    m_dbo.execute(QStringLiteral("insert into no_such_table values (1)"));
    QVERIFY(m_dbo.batchFailed());
    QVERIFY(!m_dbo.commitBatch());
    QVERIFY(!m_dbo.inBatch());
    QVERIFY(!exists(QStringLiteral("batch-d")));
}

int main(int argc, char **argv)
{
    // QApplication-free: DatabaseObjects only shows dialogs when opened with gui=true.
    QCoreApplication application(argc, argv);
    RecordTreeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_recordtree.moc"

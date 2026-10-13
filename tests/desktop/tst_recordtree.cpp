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

int main(int argc, char **argv)
{
    // QApplication-free: DatabaseObjects only shows dialogs when opened with gui=true.
    QCoreApplication application(argc, argv);
    RecordTreeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_recordtree.moc"

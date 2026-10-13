// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Headless coverage for the QTextDocument/QPdfWriter report renderer. Set
// PN_RENDER_OUT=<dir> to keep the generated PDFs for visual review.

#include "ProjectNotesEmail/MeetingNotesEmailBuilder.h"
#include "ProjectNotesEmail/MeetingNotesReportBuilder.h"
#include "ProjectNotesEmail/StatusReportBuilder.h"
#include "ProjectNotesEmail/TrackerItemsReportBuilder.h"
#include "ProjectNotesEmailRendering/TextDocumentReportRenderer.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace PN::Comm;

namespace {

TrackerItemsReportInput trackerInput(int count)
{
    TrackerItemsReportInput input;
    input.projectNumber = QStringLiteral("P4000");
    input.projectName = QStringLiteral("IT New Server Install");
    const QStringList statuses {QStringLiteral("New"), QStringLiteral("Assigned"), QStringLiteral("Resolved")};
    const QStringList priorities {QStringLiteral("High"), QStringLiteral("Medium"), QStringLiteral("Low")};
    for (int i = 0; i < count; ++i) {
        TrackerReportItem item;
        item.number = QString::number(1000 + i);
        item.name = QStringLiteral("Item %1 with a reasonably descriptive name").arg(i);
        item.identifiedBy = QStringLiteral("Jim Smith");
        item.dateIdentified = QStringLiteral("09/02/2026");
        item.description = (i % 7 == 0)
            ? QStringLiteral("AVeryLongUnbrokenIdentifierThatWouldOverflowAFixedWidthColumnIfWrappingWereOff_%1").arg(i)
            : QStringLiteral("Server rack delivery is delayed by the vendor.\nFollow up with purchasing.");
        item.assignedTo = QStringLiteral("Mike Smith");
        item.priority = priorities.at(i % 3);
        item.status = statuses.at(i % 2);
        item.dueDate = QStringLiteral("09/%1/2026").arg(10 + i % 18, 2, 10, QLatin1Char('0'));
        item.lastUpdate = QStringLiteral("09/11/2026");
        item.comments = QStringLiteral("Vendor confirmed new ship date.");
        item.itemType = QStringLiteral("Tracker");
        input.items.append(item);
    }
    input.options.reportingDate = QDate(2026, 9, 12);
    input.options.tracker = TrackerFilters{};
    return input;
}

StatusReportInput statusInput()
{
    StatusReportInput input;
    input.projectNumber = QStringLiteral("P4000");
    input.projectName = QStringLiteral("IT New Server Install");
    input.managerName = QStringLiteral("My Name");
    input.reportingPeriod = QStringLiteral("Weekly");
    input.budget = QStringLiteral("$2,090.00");
    input.actual = QStringLiteral("$649.00");
    input.bcwp = QStringLiteral("$649.00");
    input.bcws = QStringLiteral("$2,090.00");
    input.bac = QStringLiteral("$2,090.00");
    input.stakeholders = {QStringLiteral("Jim Smith"), QStringLiteral("Mike Smith")};
    input.activitiesInProgress = {QStringLiteral("Install racks"), QStringLiteral("Configure network")};
    input.activitiesNextPeriod = {QStringLiteral("Migrate file shares")};
    input.activitiesCompleted = {QStringLiteral("Site survey")};
    input.issues = {{QStringLiteral("Rack delivery"), QStringLiteral("Mike Smith"), QStringLiteral("High"),
                     QStringLiteral("09/20/2026"), QStringLiteral("Assigned")}};
    input.reportingDate = QDate(2026, 9, 12);
    return input;
}

CommunicationSnapshot meetingSnapshot()
{
    CommunicationSnapshot snapshot;
    snapshot.projectNumber = QStringLiteral("P4000");
    snapshot.projectName = QStringLiteral("IT New Server Install");
    snapshot.contacts = {{QStringLiteral("a"), QStringLiteral("Alice"), {}, {}, {}, false, true}};
    snapshot.notes = {{QStringLiteral("note"), QStringLiteral("Kickoff"),
                       QStringLiteral("<p>Discussed <b>scope</b> and <span style='background-color:#ffff00'>risks</span>.</p>"
                                      "<ul><li>Racks</li><li>Network</li></ul>"
                                      "<table border='1'><tr><td>a</td><td>b</td></tr></table>"),
                       QDateTime(QDate(2026, 9, 10), QTime(9, 0)), false, {QStringLiteral("a")}}};
    snapshot.actionItems = {{QStringLiteral("note"), QStringLiteral("Order racks"), QStringLiteral("Alice"),
                             QStringLiteral("Assigned"), QStringLiteral("09/20/2026")}};
    return snapshot;
}

// 1x1 transparent PNG.
const char kPngBase64[] =
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==";

} // namespace

class TextDocumentRendererTest final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void rendersEachWorkflow_data();
    void rendersEachWorkflow();
    void appliesPageLayout();
    void asyncRenderCompletesAndCancels();
    void rejectsInvalidRequests();
    void blocksNonDataResources();
    void largeTrackerReportIsFast();

private:
    QString outputPath(const QString &name);
    static RenderRequest request(const ReportDocument &document, const QString &path);
    static int pageCount(const QString &pdf);
    QTemporaryDir m_dir;
};

void TextDocumentRendererTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
}

QString TextDocumentRendererTest::outputPath(const QString &name)
{
    const QString keep = qEnvironmentVariable("PN_RENDER_OUT");
    const QString dir = keep.isEmpty() ? m_dir.path() : keep;
    const QString path = dir + QLatin1Char('/') + name + QStringLiteral(".pdf");
    QFile::remove(path);
    return path;
}

RenderRequest TextDocumentRendererTest::request(const ReportDocument &document, const QString &path)
{
    RenderRequest request;
    request.operationId = QUuid::createUuid();
    request.previewRevision = 1;
    request.html = document.htmlDocument;
    request.outputStem = document.fileStem;
    request.pageLayout = document.pdfLayout;
    request.outputPdfPath = path;
    return request;
}

int TextDocumentRendererTest::pageCount(const QString &pdf)
{
    QFile file(pdf);
    if (!file.open(QIODevice::ReadOnly))
        return -1;
    // QPdfWriter writes one "/Type /Page" object per page (and one /Pages tree).
    const QByteArray bytes = file.readAll();
    static const QRegularExpression page(QStringLiteral("/Type\\s*/Page[^s]"));
    return int(QString::fromLatin1(bytes).count(page));
}

void TextDocumentRendererTest::rendersEachWorkflow_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("html");
    QTest::addColumn<QPageLayout>("layout");

    const auto tracker = TrackerItemsReportBuilder::build(trackerInput(40));
    const auto status = StatusReportBuilder::build(statusInput());
    MeetingNotesReportInput meetingReport;
    meetingReport.snapshot = meetingSnapshot();
    meetingReport.reportingDate = QDate(2026, 9, 12);
    const auto meetings = MeetingNotesReportBuilder::build(meetingReport);
    MeetingNotesBuildInput meetingNote;
    meetingNote.snapshot = meetingSnapshot();
    meetingNote.noteId = QStringLiteral("note");
    const auto note = MeetingNotesEmailBuilder::build(meetingNote);
    QVERIFY(tracker && status && meetings && note);

    QTest::newRow("tracker") << QStringLiteral("tracker-items") << tracker->htmlDocument << tracker->pdfLayout;
    QTest::newRow("status") << QStringLiteral("status-report") << status->htmlDocument << status->pdfLayout;
    QTest::newRow("meetings") << QStringLiteral("meeting-notes-report") << meetings->htmlDocument << meetings->pdfLayout;
    QTest::newRow("note") << QStringLiteral("send-meeting-notes") << note->htmlDocument << note->pdfLayout;
}

void TextDocumentRendererTest::rendersEachWorkflow()
{
    QFETCH(QString, name);
    QFETCH(QString, html);
    QFETCH(QPageLayout, layout);

    ReportDocument document;
    document.htmlDocument = html;
    document.pdfLayout = layout;
    document.fileStem = name;
    const QString path = outputPath(name);
    QCOMPARE(TextDocumentReportRenderer::renderToFile(request(document, path)), QString());
    QVERIFY(QFileInfo(path).size() > 1000);
    QVERIFY(pageCount(path) >= 1);
}

void TextDocumentRendererTest::appliesPageLayout()
{
    ReportDocument document;
    document.htmlDocument = QStringLiteral("<html><body><p>Landscape</p></body></html>");
    document.pdfLayout = QPageLayout(QPageSize(QPageSize::Letter), QPageLayout::Landscape,
                                     QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
    const QString path = outputPath(QStringLiteral("layout"));
    QCOMPARE(TextDocumentReportRenderer::renderToFile(request(document, path)), QString());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    // Letter landscape is 792 x 612 points.
    static const QRegularExpression mediaBox(QStringLiteral("/MediaBox \\[0 0 792(\\.0+)? 612(\\.0+)?\\]"));
    QVERIFY(QString::fromLatin1(file.readAll()).contains(mediaBox));
}

void TextDocumentRendererTest::asyncRenderCompletesAndCancels()
{
    TextDocumentReportRenderer renderer;
    const auto status = StatusReportBuilder::build(statusInput());
    QVERIFY(status);

    bool completed = false;
    RenderResult result;
    const QString path = outputPath(QStringLiteral("async"));
    renderer.render(request(*status, path), [&](RenderResult value) { result = std::move(value); completed = true; });
    QTRY_VERIFY_WITH_TIMEOUT(completed, 20000);
    QVERIFY2(result.error.code.isEmpty(), qPrintable(result.error.code));
    QCOMPARE(result.pdf.absolutePath, path);
    QCOMPARE(result.pdf.mimeType, QStringLiteral("application/pdf"));
    QVERIFY(result.pdf.byteSize > 0);

    completed = false;
    const QString cancelledPath = outputPath(QStringLiteral("cancelled"));
    RenderRequest cancelled = request(*status, cancelledPath);
    renderer.render(cancelled, [&](RenderResult value) { result = std::move(value); completed = true; });
    renderer.cancel(cancelled.operationId);
    QTRY_VERIFY_WITH_TIMEOUT(completed, 20000);
    QCOMPARE(result.error.code, QStringLiteral("operation-cancelled"));
    QVERIFY(!QFileInfo::exists(cancelledPath));
}

void TextDocumentRendererTest::rejectsInvalidRequests()
{
    TextDocumentReportRenderer renderer;
    RenderResult result;
    bool completed = false;
    RenderRequest bad;
    bad.operationId = QUuid::createUuid();
    bad.html = QStringLiteral("<p>x</p>");
    bad.outputPdfPath = QStringLiteral("relative.pdf");
    renderer.render(bad, [&](RenderResult value) { result = std::move(value); completed = true; });
    QVERIFY(completed);
    QCOMPARE(result.error.code, QStringLiteral("renderer-input-invalid"));
}

void TextDocumentRendererTest::blocksNonDataResources()
{
    // A file the document must not read, referenced as an image.
    const QString secretImage = m_dir.path() + QStringLiteral("/secret.png");
    QImage image(40, 40, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QVERIFY(image.save(secretImage));

    const QString html = QStringLiteral(
        "<html><body>"
        "<img src='file://%1' width='40' height='40'>"
        "<img src='%1' width='40' height='40'>"
        "<img src='qrc:/qt-project.org/qmessagebox/images/qtlogo-64.png'>"
        "<img src='https://example.com/x.png'>"
        "<img src='data:image/png;base64,%2' width='10' height='10'>"
        "</body></html>").arg(secretImage, QString::fromLatin1(kPngBase64));

    // Blocked images become 1x1 transparent placeholders and the data: image
    // is 1x1, so every image XObject in the PDF must be 1 pixel wide. The
    // blocked file image is 40x40 and the qrc logo 64x64.
    ReportDocument withImages;
    withImages.htmlDocument = html;
    const QString path = outputPath(QStringLiteral("resources"));
    QCOMPARE(TextDocumentReportRenderer::renderToFile(request(withImages, path)), QString());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray bytes = file.readAll();
    static const QRegularExpression width(QStringLiteral("/Width\\s+(\\d+)"));
    auto it = width.globalMatch(QString::fromLatin1(bytes));
    int images = 0;
    while (it.hasNext()) {
        QCOMPARE(it.next().captured(1), QStringLiteral("1"));
        ++images;
    }
    QVERIFY(images >= 1);
}

void TextDocumentRendererTest::largeTrackerReportIsFast()
{
    const auto tracker = TrackerItemsReportBuilder::build(trackerInput(500));
    QVERIFY(tracker);
    const QString path = outputPath(QStringLiteral("tracker-500"));
    QElapsedTimer timer;
    timer.start();
    QCOMPARE(TextDocumentReportRenderer::renderToFile(request(*tracker, path)), QString());
    const qint64 elapsed = timer.elapsed();
    qInfo("500-item tracker report: %lld ms, %d pages", elapsed, pageCount(path));
    QVERIFY(pageCount(path) > 5);
    QVERIFY2(elapsed < 15000, "rendering a 500-item tracker report took too long");
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    TextDocumentRendererTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_textdocumentrenderer.moc"

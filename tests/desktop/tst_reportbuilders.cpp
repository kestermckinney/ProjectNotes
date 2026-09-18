// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectNotesEmail/MeetingNotesEmailBuilder.h"
#include "ProjectNotesEmail/MeetingNotesReportBuilder.h"
#include "ProjectNotesEmail/StatusReportBuilder.h"
#include "ProjectNotesEmail/TrackerItemsReportBuilder.h"

#include <QtTest/QtTest>

using namespace PN::Comm;

class ReportBuildersTest final : public QObject {
    Q_OBJECT

private slots:
    void buildsContextSpecificMeetingNotes();
    void filtersProjectWideMeetingNotes();
    void buildsStatusReportWithEscapedValuesAndLetterLayout();
    void filtersAndSortsTrackerReportWithLandscapeLayout();
};

void ReportBuildersTest::buildsContextSpecificMeetingNotes()
{
    MeetingNotesBuildInput input;
    input.snapshot.projectNumber = QStringLiteral("P-1");
    input.snapshot.projectName = QStringLiteral("North");
    input.snapshot.people = {{QStringLiteral("a"), QStringLiteral("Alice"), {}, {}, {}, false, true}};
    input.snapshot.notes = {{QStringLiteral("note"), QStringLiteral("Kickoff"),
                             QStringLiteral("<p>Discussed <b>scope</b></p>"),
                             QDateTime(QDate(2026, 9, 12), QTime(9, 0)), false,
                             {QStringLiteral("a")}}};
    input.noteId = QStringLiteral("note");
    input.snapshot.actionItems = {{QStringLiteral("note"), QStringLiteral("Draft"),
                                   QStringLiteral("Alice"), QStringLiteral("Assigned"),
                                   QStringLiteral("09/20/2026")}};

    ValidationResult validation;
    const auto document = MeetingNotesEmailBuilder::build(input, &validation);
    QVERIFY(validation.ok());
    QVERIFY(document.has_value());
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Discussed <b>scope</b>")));
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Alice")));
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Draft")));
    QCOMPARE(document->defaultSubject, QStringLiteral("P-1 North - 09/12/2026 Kickoff Notes"));
    QCOMPARE(document->pdfLayout.pageSize().id(), QPageSize::A4);
    QCOMPARE(document->pdfLayout.orientation(), QPageLayout::Portrait);
}

void ReportBuildersTest::filtersProjectWideMeetingNotes()
{
    MeetingNotesReportInput input;
    input.snapshot.projectNumber = QStringLiteral("P-1");
    input.snapshot.projectName = QStringLiteral("North");
    input.snapshot.notes = {{QStringLiteral("external"), QStringLiteral("External"),
                             QStringLiteral("<p>Public</p>"), QDateTime(QDate(2026, 9, 10), {}), false},
                            {QStringLiteral("internal"), QStringLiteral("Internal"),
                             QStringLiteral("<p>Private</p>"), QDateTime(QDate(2026, 9, 11), {}), true},
                            {QStringLiteral("future"), QStringLiteral("Future"),
                             QStringLiteral("<p>Later</p>"), QDateTime(QDate(2026, 9, 13), {}), false}};
    input.snapshot.actionItems = {{QStringLiteral("external"), QStringLiteral("Follow up"),
                                  QStringLiteral("Alice"), QStringLiteral("New"), QStringLiteral("09/14/2026")}};
    input.reportingDate = QDate(2026, 9, 12);

    ValidationResult validation;
    const auto external = MeetingNotesReportBuilder::build(input, &validation);
    QVERIFY(validation.ok());
    QVERIFY(external.has_value());
    QVERIFY(external->htmlDocument.contains(QStringLiteral("<h1>Meeting Notes: P-1 North</h1>")));
    QVERIFY(external->htmlDocument.contains(QStringLiteral("class='meeting'")));
    QVERIFY(external->htmlDocument.contains(QStringLiteral("class='meeting-table'")));
    QVERIFY(external->htmlDocument.contains(QStringLiteral("Action Items")));
    QVERIFY(external->htmlDocument.contains(QStringLiteral("Created by Project Notes")));
    QVERIFY(external->htmlDocument.contains(QStringLiteral("Public")));
    QVERIFY(external->htmlDocument.contains(QStringLiteral("Report Date: 09/12/2026")));
    QVERIFY(!external->htmlDocument.contains(QStringLiteral("Private")));
    QVERIFY(!external->htmlDocument.contains(QStringLiteral("Later")));
    QCOMPARE(external->defaultSubject, QStringLiteral("P-1 North - 09/12/2026"));
    QCOMPARE(external->fileStem, QStringLiteral("P-1 Meeting Minutes"));

    input.internalReport = true;
    const auto internal = MeetingNotesReportBuilder::build(input);
    QVERIFY(internal.has_value());
    QVERIFY(internal->htmlDocument.contains(QStringLiteral("Private")));
    QCOMPARE(internal->defaultSubject, QStringLiteral("P-1 North - 09/12/2026"));
    QVERIFY(internal->fileStem.endsWith(QStringLiteral(" Internal")));

    input.internalReport = false;
    input.reportingDate = QDate(2026, 9, 10);
    const auto earlierExternal = MeetingNotesReportBuilder::build(input);
    QVERIFY(earlierExternal.has_value());
    QVERIFY(earlierExternal->htmlDocument.contains(QStringLiteral("Public")));
    QVERIFY(!earlierExternal->htmlDocument.contains(QStringLiteral("Private")));
    QCOMPARE(earlierExternal->defaultSubject, QStringLiteral("P-1 North - 09/10/2026"));
}

void ReportBuildersTest::buildsStatusReportWithEscapedValuesAndLetterLayout()
{
    StatusReportInput input;
    input.projectNumber = QStringLiteral("P-1");
    input.projectName = QStringLiteral("North");
    input.managerName = QStringLiteral("Manager");
    input.reportingPeriod = QStringLiteral("Weekly");
    input.reportingDate = QDate(2026, 9, 15);
    input.actual = QStringLiteral("$25");
    input.bcwp = QStringLiteral("$30");
    input.bcws = QStringLiteral("$35");
    input.bac = QStringLiteral("$100");
    input.stakeholders = {QStringLiteral("Alice"), QStringLiteral("Bob")};
    input.activitiesInProgress = {QStringLiteral("Build <review>")};
    input.issues = {{QStringLiteral("Risk <one>"), QStringLiteral("Alice"),
                     QStringLiteral("High"), QStringLiteral("09/20/2026"), QStringLiteral("New")},
                    {QStringLiteral("Internal issue"), QStringLiteral("Bob"),
                     QStringLiteral("Low"), QStringLiteral("09/21/2026"), QStringLiteral("New"), true}};

    ValidationResult validation;
    const auto document = StatusReportBuilder::build(input, &validation);
    QVERIFY(validation.ok());
    QVERIFY(document.has_value());
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Risk &lt;one&gt;")));
    QVERIFY(!document->htmlDocument.contains(QStringLiteral("Internal issue")));
    QVERIFY(document->htmlDocument.contains(QStringLiteral("-16.67%")));
    QVERIFY(document->htmlDocument.contains(QStringLiteral("<td class='cell-label'>Date:</td><td class='cell-value'>09/15/2026</td>")));
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Report Date: 09/15/2026")));
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Created by Project Notes")));
    // The email body must carry the same styling as the exported report.
    QVERIFY(document->emailFragment.startsWith(QStringLiteral("<style>")));
    QVERIFY(document->emailFragment.contains(QStringLiteral("background:#DCE6F1")));
    QVERIFY(document->emailFragment.contains(QStringLiteral(".ev-columns{display:flex")));
    QVERIFY(document->htmlDocument.contains(document->emailFragment));
    QVERIFY(!document->plainText.contains(QStringLiteral("font-family")));
    QCOMPARE(document->defaultSubject, QStringLiteral("P-1 North - Status Report 09/15/2026"));
    QCOMPARE(document->pdfLayout.pageSize().id(), QPageSize::Letter);
    QCOMPARE(document->pdfLayout.orientation(), QPageLayout::Portrait);
    QCOMPARE(document->pdfLayout.margins(QPageLayout::Millimeter).left(), 20.0);

    input.internalReport = true;
    const auto internalDocument = StatusReportBuilder::build(input, &validation);
    QVERIFY(validation.ok());
    QVERIFY(internalDocument.has_value());
    QVERIFY(internalDocument->htmlDocument.contains(QStringLiteral("Internal issue")));

    input.issues.clear();
    const auto noIssuesDocument = StatusReportBuilder::build(input, &validation);
    QVERIFY(validation.ok());
    QVERIFY(noIssuesDocument.has_value());
    QVERIFY(!noIssuesDocument->htmlDocument.contains(QStringLiteral("No issues.")));
}

void ReportBuildersTest::filtersAndSortsTrackerReportWithLandscapeLayout()
{
    TrackerItemsReportInput input;
    input.projectNumber = QStringLiteral("P-1");
    input.projectName = QStringLiteral("North");
    input.options = defaultReportOptions(Workflow::TrackerItemsReport, QDate(2026, 9, 15));
    input.options.tracker->itemTypes = {QStringLiteral("Tracker"), QStringLiteral("Action")};
    input.options.tracker->statuses = {QStringLiteral("New"), QStringLiteral("Assigned")};
    input.items = {{QStringLiteral("2"), QStringLiteral("Medium <item>"), {}, {}, {}, {},
                    QStringLiteral("Medium"), QStringLiteral("New"), QStringLiteral("09/20/2026"),
                    {}, {}, {}, QStringLiteral("Tracker"), false},
                   {QStringLiteral("1"), QStringLiteral("Earlier high"), {}, {}, {}, {},
                    QStringLiteral("High"), QStringLiteral("New"), QStringLiteral("09/19/2026"),
                    {}, {}, {}, QStringLiteral("Tracker"), false},
                   {QStringLiteral("5"), QStringLiteral("Later high"), {}, {}, {}, {},
                    QStringLiteral("High"), QStringLiteral("New"), QStringLiteral("09/21/2026"),
                    {}, {}, {}, QStringLiteral("Tracker"), false},
                   {QStringLiteral("6"), QStringLiteral("Undated high"), {}, {}, {}, {},
                    QStringLiteral("High"), QStringLiteral("New"), {},
                    {}, {}, {}, QStringLiteral("Tracker"), false},
                   {QStringLiteral("3"), QStringLiteral("Hidden"), {}, {}, {}, {},
                    QStringLiteral("High"), QStringLiteral("Resolved"), QStringLiteral("09/25/2026"),
                    {}, {}, {}, QStringLiteral("Tracker"), false},
                   {QStringLiteral("4"), QStringLiteral("Internal"), {}, {}, {}, {},
                    QStringLiteral("High"), QStringLiteral("Assigned"), QStringLiteral("09/21/2026"),
                    {}, {}, {}, QStringLiteral("Action"), true}};

    ValidationResult validation;
    const auto document = TrackerItemsReportBuilder::build(input, &validation);
    QVERIFY(validation.ok());
    QVERIFY(document.has_value());
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Medium &lt;item&gt;")));
    QVERIFY(!document->htmlDocument.contains(QStringLiteral("Hidden")));
    QVERIFY(!document->htmlDocument.contains(QStringLiteral("Internal")));
    QVERIFY(document->htmlDocument.indexOf(QStringLiteral("Later high"))
            < document->htmlDocument.indexOf(QStringLiteral("Earlier high")));
    QVERIFY(document->htmlDocument.indexOf(QStringLiteral("Earlier high"))
            < document->htmlDocument.indexOf(QStringLiteral("Undated high")));
    QVERIFY(document->htmlDocument.indexOf(QStringLiteral("Undated high"))
            < document->htmlDocument.indexOf(QStringLiteral("Medium &lt;item&gt;")));
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Report Date: 09/15/2026")));
    QVERIFY(document->htmlDocument.contains(QStringLiteral("Created by Project Notes")));
    QCOMPARE(document->defaultSubject, QStringLiteral("P-1 North - Tracker Items 09/15/2026"));
    QCOMPARE(document->pdfLayout.pageSize().id(), QPageSize::Letter);
    QCOMPARE(document->pdfLayout.orientation(), QPageLayout::Landscape);
    QCOMPARE(document->pdfLayout.margins(QPageLayout::Millimeter).left(), 12.0);

    input.options.internalReport = true;
    const auto internal = TrackerItemsReportBuilder::build(input, &validation);
    QVERIFY(validation.ok());
    QVERIFY(internal.has_value());
    QVERIFY(internal->htmlDocument.contains(QStringLiteral("<td>Y</td>")));

    input.items.clear();
    const auto internalEmpty = TrackerItemsReportBuilder::build(input, &validation);
    QVERIFY(internalEmpty.has_value());
    QVERIFY(internalEmpty->htmlDocument.contains(QStringLiteral("<th>Internal</th>")));
    QVERIFY(internalEmpty->htmlDocument.contains(
        QStringLiteral("<td colspan='13'>No matching tracker items.</td>")));
}

QTEST_GUILESS_MAIN(ReportBuildersTest)

#include "tst_reportbuilders.moc"

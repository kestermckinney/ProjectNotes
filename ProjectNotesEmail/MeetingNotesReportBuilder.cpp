// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingNotesReportBuilder.h"

#include <QPageSize>
#include <QRegularExpression>

namespace PN::Comm {
namespace {
QString escape(const QString &value) { return value.toHtmlEscaped(); }
QString body(const QString &html)
{
    const QRegularExpression expression(R"(<body[^>]*>(.*)</body>)",
                                         QRegularExpression::CaseInsensitiveOption
                                             | QRegularExpression::DotMatchesEverythingOption);
    const auto match = expression.match(html);
    return match.hasMatch() ? match.captured(1) : html;
}

QString meetingBlock(const CommunicationSnapshot &snapshot, const SnapshotNote &note)
{
    QStringList attendees;
    for (const SnapshotPerson &person : snapshot.people)
        if (note.attendeeIds.contains(person.id) && !person.name.trimmed().isEmpty())
            attendees.append(person.name);

    QString actions;
    for (const SnapshotActionItem &item : snapshot.actionItems) {
        if (item.noteId != note.id)
            continue;
        actions += QStringLiteral("<tr><td class='cell-value'>%1</td><td class='cell-value' style='text-align:center'>%2</td>"
                                  "<td class='cell-value' style='text-align:center'>%3</td><td class='cell-value' style='text-align:center'>%4</td></tr>")
                       .arg(escape(item.name), escape(item.assignedTo), escape(item.status), escape(item.dueDate));
    }
    if (!actions.isEmpty())
        actions.prepend(QStringLiteral("<tr><td colspan='4' class='cell-header'>Action Items</td></tr>"
                                       "<tr><th class='cell-header col-item'>Item</th><th class='cell-header'>Assigned To</th>"
                                       "<th class='cell-header'>Status</th><th class='cell-header'>Due Date</th></tr>"));
    return QStringLiteral("<div class='meeting'><div class='meeting-title'>%1</div><table class='meeting-table'>"
                          "<tr><td class='cell-label'>Date:</td><td class='cell-value' colspan='3'>%2</td></tr>"
                          "<tr><td class='cell-label'>Attendees:</td><td class='cell-value' colspan='3'>%3</td></tr>"
                          "<tr><td colspan='4' class='cell-header'>Meeting Notes</td></tr>"
                          "<tr><td colspan='4' class='cell-notes'>%4</td></tr>%5</table></div>")
        .arg(escape(note.title), escape(note.date.date().toString("MM/dd/yyyy")), escape(attendees.join(", ")),
             body(note.html), actions);
}
} // namespace

std::optional<ReportDocument> MeetingNotesReportBuilder::build(const MeetingNotesReportInput &input,
                                                               ValidationResult *validation)
{
    if (!input.reportingDate.isValid()) {
        if (validation) validation->addError("reporting-date-required", "reportingDate");
        return std::nullopt;
    }
    QStringList sections;
    for (const SnapshotNote &note : input.snapshot.notes) {
        if (!note.date.isValid() || note.date.date() > input.reportingDate || (note.internal && !input.internalReport)) continue;
        sections.append(meetingBlock(input.snapshot, note));
    }
    if (sections.isEmpty()) {
        if (validation) validation->addError("meeting-notes-empty", "reportingDate");
        return std::nullopt;
    }
    ReportDocument report; report.workflow = Workflow::MeetingNotesReport;
    const QString date = input.reportingDate.toString("MM/dd/yyyy");
    report.emailFragment = QStringLiteral("<h1>Meeting Notes: %1</h1>%2<p class='footer'>Report Date: %3</p>"
                                          "<p class='branding'>Created by Project Notes (<a href='https://www.projectnotespro.com'>www.projectnotespro.com</a>)</p>")
        .arg(escape(input.snapshot.projectNumber + QStringLiteral(" ") + input.snapshot.projectName),
             sections.join(QString()), escape(date));
    report.htmlDocument = QStringLiteral("<!doctype html><html><head><meta charset='utf-8'><style>"
        "body{font-family:Calibri,Arial,sans-serif;font-size:10pt;margin:0;padding:.2in}h1{font-size:13pt;color:#1F497D;font-weight:bold;margin-bottom:8px}"
        ".meeting{margin-bottom:16px}.meeting-title{font-size:11pt;font-weight:bold;color:#1F497D;margin-bottom:4px}"
        ".meeting-table{border-collapse:collapse;width:100%}.meeting-table td,.meeting-table th{border:1px solid #808080;padding:3px 6px;font-size:10pt;vertical-align:top}"
        ".cell-label{background:#EEECE1;font-weight:bold;text-align:right;white-space:nowrap;width:80px}.cell-value,.cell-notes{background:#DCE6F1}.cell-header{background:#EEECE1;font-weight:bold;text-align:center}.col-item{width:55%}"
        ".footer{font-size:9pt;color:#555;margin-top:16px}.branding{font-size:9pt;color:#000;margin-top:2px}.branding a{color:#0066CC;text-decoration:none}</style></head><body>%1</body></html>").arg(report.emailFragment);
    QString plain = report.emailFragment; plain.remove(QRegularExpression("<[^>]*>")); report.plainText = plain.simplified();
    report.defaultSubject = QStringLiteral("%1 %2 - %3").arg(input.snapshot.projectNumber, input.snapshot.projectName, date);
    report.fileStem = input.snapshot.projectNumber + QStringLiteral(" Meeting Minutes")
        + (input.internalReport ? QStringLiteral(" Internal") : QString());
    report.pdfLayout = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait,
                                   QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
    return report;
}

} // namespace PN::Comm

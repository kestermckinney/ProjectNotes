// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingNotesEmailBuilder.h"

#include <QPageSize>
#include <QRegularExpression>
#include <QTextDocument>

namespace PN::Comm {
namespace {
QString escape(const QString &value) { return value.toHtmlEscaped(); }
QString body(const QString &html) {
    const QRegularExpression expression(R"(<body[^>]*>(.*)</body>)",
                                         QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const auto match = expression.match(html); return match.hasMatch() ? match.captured(1) : html;
}
QString plain(const QString &html) { QString value = html; value.remove(QRegularExpression("<[^>]*>")); return value.simplified(); }
QString mobilePlain(const QString &html)
{
    if (html.isEmpty() || !html.contains('<')) return html;
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText();
}

// Email clients receive the fragment without the exported document's head.
// Style only our markup, before interpolating any user-authored note HTML.
QString emailMarkup(QString markup)
{
    const QString cell = "font-family:Calibri,Arial,sans-serif;border:1px solid #808080;"
                         "padding:6px;vertical-align:top;";
    markup.replace("<table class='meeting-notes'>",
                   "<table class='meeting-notes' width='100%' cellspacing='0' cellpadding='0' "
                   "style='border-collapse:collapse;width:100%;'>");
    markup.replace(QRegularExpression("<th(?=[ >])"),
                   "<th bgcolor='#e7e6e6' style='" + cell + "background-color:#e7e6e6;'");
    markup.replace("<td colspan='4' class='note-body'>",
                   "<td colspan='4' class='note-body' bgcolor='#d9e1f2' style='" + cell
                       + "background-color:#d9e1f2;'>");
    markup.replace("<td>", "<td style='" + cell + "'>");
    markup.replace("<td colspan='3'>", "<td colspan='3' style='" + cell + "'>");
    markup.replace("<td colspan='4'>", "<td colspan='4' style='" + cell + "'>");
    return markup;
}
}

std::optional<ReportDocument> MeetingNotesEmailBuilder::build(const MeetingNotesBuildInput &input,
                                                              ValidationResult *validation)
{
    const auto note = std::find_if(input.snapshot.notes.cbegin(), input.snapshot.notes.cend(),
                                   [&input](const SnapshotNote &value) { return value.id == input.noteId; });
    if (note == input.snapshot.notes.cend()) {
        if (validation) validation->addError("meeting-note-not-found", "noteId");
        return std::nullopt;
    }
    QStringList attendees;
    for (const SnapshotPerson &person : input.snapshot.people)
        if (note->attendeeIds.contains(person.id) && !person.name.trimmed().isEmpty()) attendees.append(person.name);
    QList<MeetingActionItem> items = input.actionItems;
    if (items.isEmpty()) for (const SnapshotActionItem &item : input.snapshot.actionItems)
        if (item.noteId == note->id) items.append({item.name, item.assignedTo, item.status, item.dueDate});
    QString actions;
    for (const MeetingActionItem &item : items)
        actions += emailMarkup(QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>"))
                       .arg(escape(item.name), escape(item.assignedTo), escape(item.status), escape(item.dueDate));
    if (actions.isEmpty()) actions = emailMarkup(QStringLiteral("<tr><td colspan='4'>No action items.</td></tr>"));
    const QString date = note->date.isValid() ? note->date.date().toString("MM/dd/yyyy") : QString();
    const QString fragment = emailMarkup(QStringLiteral("<table class='meeting-notes'><tr><th colspan='4'>%1 - %2</th></tr>"
        "<tr><th>Meeting Date</th><td colspan='3'>%3</td></tr><tr><th>Attendees</th><td colspan='3'>%4</td></tr>"
        "<tr><th colspan='4'>Meeting Notes</th></tr><tr><td colspan='4' class='note-body'>%5</td></tr>"
        "<tr><th colspan='4'>Action Items</th></tr><tr><th>Item</th><th>Assigned To</th><th>Status</th><th>Due</th></tr>%6</table>"))
        .arg(escape(input.snapshot.projectNumber + QStringLiteral(" ") + input.snapshot.projectName),
             escape(note->title), escape(date), escape(attendees.join(", ")), body(note->html), actions);
    QString mobileText = mobilePlain(note->html);
    QStringList actionText;
    for (const MeetingActionItem &item : items) {
        QString entry = item.name;
        if (!item.assignedTo.isEmpty()) entry += QStringLiteral("  Assigned To: ") + item.assignedTo;
        if (!item.dueDate.isEmpty()) entry += QStringLiteral("  Due By: ") + item.dueDate;
        actionText.append(entry);
    }
    if (!actionText.isEmpty()) mobileText += QStringLiteral("\n\nAction Items:\n") + actionText.join(QStringLiteral("\n"));
    if (!attendees.isEmpty()) mobileText += QStringLiteral("\n\nAttendees:\n") + attendees.join(QStringLiteral("\r\n"));
    ReportDocument report; report.workflow = Workflow::SendMeetingNotes; report.emailFragment = fragment;
    report.htmlDocument = QStringLiteral("<!doctype html><html><head><meta charset='utf-8'><style>"
        "body{font-family:Calibri,sans-serif}</style></head><body>%1</body></html>").arg(fragment);
    report.plainText = mobileText; report.defaultSubject = QStringLiteral("%1 %2 - %3 %4 Notes")
        .arg(input.snapshot.projectNumber, input.snapshot.projectName, date, note->title).simplified();
    report.fileStem = QStringLiteral("Meeting Notes"); report.pdfLayout = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, {});
    return report;
}

} // namespace PN::Comm

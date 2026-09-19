#include "TrackerItemsReportBuilder.h"
#include <QPageSize>
#include <QRegularExpression>
#include <QTextDocument>
#include <algorithm>
namespace PN::Comm { namespace {
QString e(const QString&s){return s.toHtmlEscaped();}
int priority(const QString&s){if(s=="High")return 0;if(s=="Medium")return 1;if(s=="Low")return 2;return 3;}
int status(const QString&s){static const QStringList order{"New","Assigned","Resolved","Defered","Cancelled"};auto i=order.indexOf(s);return i<0?order.size():i;}

// Email bodies are embedded in another document, often without its stylesheet.
// These replacements apply only to the controlled markup generated below;
// report values have already been HTML-escaped.
QString inlineEmailFragment(QString fragment)
{
    const QString font = "font-family:Calibri,Arial,sans-serif;";
    const QString cell = font + "border:1px solid #808080;padding:3px 4px;"
        "font-size:7.5pt;word-wrap:break-word;";
    fragment.replace("<h1>", "<h1 style='" + font
        + "font-size:13pt;color:#1F497D;font-weight:bold;margin-bottom:6px;'>");
    fragment.replace("<table>", "<table width='100%' cellspacing='0' cellpadding='0' "
        "style='border-collapse:collapse;width:100%;table-layout:fixed;'>");
    const QStringList columns {"id", "item", "identby", "dateident", "desc", "assigned",
                               "priority", "status", "duedate", "lastupd", "resolved", "comments", "int"};
    const QList<int> widths {3, 11, 7, 6, 13, 7, 5, 6, 6, 6, 6, 21, 3};
    for (int i = 0; i < columns.size(); ++i) {
        const QString className = "col-" + columns[i];
        const QString width = QString::number(widths[i]) + "%";
        const QString alignment = (i == 0 || i == 3 || (i >= 6 && i <= 10) || i == 12)
            ? "center" : "left";
        fragment.replace("<th class='" + className + "'>",
            "<th class='" + className + "' width='" + width + "' bgcolor='#EEECE1' style='"
            + cell + "background-color:#EEECE1;vertical-align:middle;font-weight:bold;"
            "text-align:center;width:" + width + ";'>");
        fragment.replace("<td class='" + className + "'>",
            "<td class='" + className + "' width='" + width + "' bgcolor='#DCE6F1' style='"
            + cell + "background-color:#DCE6F1;vertical-align:top;white-space:pre-wrap;"
            "text-align:" + alignment + ";width:" + width + ";'>");
    }
    for (const auto &colspan : {QString("12"), QString("13")}) {
        fragment.replace("<tr class='group-header'><td colspan='" + colspan + "'>",
            "<tr class='group-header'><td colspan='" + colspan + "' bgcolor='#1F497D' style='"
            + cell + "background-color:#1F497D;color:#FFFFFF;font-size:8pt;font-weight:bold;'>");
        fragment.replace("<td colspan='" + colspan + "'>",
            "<td colspan='" + colspan + "' style='" + cell + "background-color:#DCE6F1;'>");
    }
    const QStringList priorities {"high", "medium", "low"};
    const QStringList colors {"#C00000", "#9C6500", "#375623"};
    for (int i = 0; i < priorities.size(); ++i)
        fragment.replace("<span class='priority-" + priorities[i] + "'>",
            "<span class='priority-" + priorities[i] + "' style='color:" + colors[i] + ";font-weight:bold;'>");
    fragment.replace("<p class='footer'>", "<p class='footer' style='" + font
        + "margin-top:8px;font-size:7.5pt;color:#666;'>");
    fragment.replace("<p class='branding'>", "<p class='branding' style='" + font
        + "font-size:9pt;color:#000000;margin-top:2px;'>");
    fragment.replace("<a href=", "<a style='color:#0066CC;text-decoration:none;' href=");
    fragment.replace("\r\n", "\n").replace("\r", "\n").replace("\n", "<br>");
    return "<div style='" + font + "font-size:9pt;margin:0;padding:0.2in;'>" + fragment + "</div>";
}

}
std::optional<ReportDocument> TrackerItemsReportBuilder::build(const TrackerItemsReportInput &in,ValidationResult *validation){
 if(!in.options.tracker){if(validation)validation->addError("tracker-filters-required","tracker");return{};}const auto &f=*in.options.tracker;QList<TrackerReportItem>items;
 for(const auto&i:in.items)if(f.itemTypes.contains(i.itemType)&&f.statuses.contains(i.status)&&(in.options.internalReport||!i.internal))items.append(i);
 std::stable_sort(items.begin(),items.end(),[](const auto&a,const auto&b){if(status(a.status)!=status(b.status))return status(a.status)<status(b.status);if(priority(a.priority)!=priority(b.priority))return priority(a.priority)<priority(b.priority);const auto ad=QDate::fromString(a.dueDate,"MM/dd/yyyy"),bd=QDate::fromString(b.dueDate,"MM/dd/yyyy");if(ad.isValid()!=bd.isValid())return ad.isValid();if(ad.isValid()&&ad!=bd)return ad>bd;return false;});
 QString rows,last;for(const auto&i:items){if(i.status!=last){last=i.status;int count=0;for(const auto&v:items)if(v.status==last)++count;rows+="<tr class='group-header'><td colspan='"+QString::number(in.options.internalReport?13:12)+"'>"+e(last)+" Items ("+QString::number(count)+")</td></tr>";}rows+="<tr class='"+(i.itemType=="Action"?QString("row-action"):QString("row-tracker"))+"'><td class='col-id'>"+e(i.number)+"</td><td class='col-item'>"+e(i.name)+"</td><td class='col-identby'>"+e(i.identifiedBy)+"</td><td class='col-dateident'>"+e(i.dateIdentified)+"</td><td class='col-desc'>"+e(i.description)+"</td><td class='col-assigned'>"+e(i.assignedTo)+"</td><td class='col-priority'><span class='priority-"+(i.priority=="High"?QString("high"):i.priority=="Medium"?QString("medium"):QString("low"))+"'>"+e(i.priority)+"</span></td><td class='col-status'>"+e(i.status)+"</td><td class='col-duedate'>"+e(i.dueDate)+"</td><td class='col-lastupd'>"+e(i.lastUpdate)+"</td><td class='col-resolved'>"+e(i.dateResolved)+"</td><td class='col-comments'>"+e(i.comments)+"</td>"+(in.options.internalReport?"<td class='col-int'>"+QString(i.internal?"Y":"N")+"</td>":"")+"</tr>";}
 if(rows.isEmpty())rows="<tr><td colspan='"+QString::number(in.options.internalReport?13:12)+"'>No matching tracker items.</td></tr>";const QString date=in.options.reportingDate.toString("MM/dd/yyyy");const QString internal = in.options.internalReport ? "<th class='col-int'>Int</th>" : "";
 const QString fragment = "<h1>Item Tracker: " + e(in.projectNumber) + " " + e(in.projectName)
     + "</h1><table><thead><tr>"
     "<th class='col-id'>ID</th><th class='col-item'>Item</th>"
     "<th class='col-identby'>Identified By</th><th class='col-dateident'>Date Identified</th>"
     "<th class='col-desc'>Description</th><th class='col-assigned'>Assigned To</th>"
     "<th class='col-priority'>Priority</th><th class='col-status'>Status</th>"
     "<th class='col-duedate'>Due Date</th><th class='col-lastupd'>Last Update</th>"
     "<th class='col-resolved'>Date Resolved</th><th class='col-comments'>Comments/Resolution</th>"
     + internal + "</tr></thead><tbody>" + rows
     + "</tbody></table><p class='footer'>Report Date: " + e(date)
     + "</p><p class='branding'>Created by Project Notes (<a href='https://www.projectnotespro.com'>www.projectnotespro.com</a>)</p>";
 ReportDocument r;
 r.workflow = Workflow::TrackerItemsReport;
 r.emailFragment = inlineEmailFragment(fragment);
 // Keep the exported report's presentation consistent with exporttrackeritems_plugin.py.
 r.htmlDocument = QStringLiteral(R"HTML(<!DOCTYPE html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=1024">
<style>
body {
    font-family: Calibri, Arial, sans-serif;
    font-size: 9pt;
    margin: 0;
    padding: 0.2in;
}
h1 {
    font-size: 13pt;
    color: #1F497D;
    font-weight: bold;
    margin-bottom: 6px;
}
table {
    border-collapse: collapse;
    width: 100%;
    table-layout: fixed;
}
tr {
    page-break-inside: avoid;
    break-inside: avoid;
}
th {
    background-color: #EEECE1;
    border: 1px solid #808080;
    padding: 3px 4px;
    text-align: center;
    font-size: 7.5pt;
    font-weight: bold;
    vertical-align: middle;
    word-wrap: break-word;
}
td {
    background-color: #DCE6F1;
    border: 1px solid #808080;
    padding: 3px 4px;
    vertical-align: top;
    font-size: 7.5pt;
    word-wrap: break-word;
    white-space: pre-wrap;
}
.col-id        { width: 3%; text-align: center; }
.col-item      { width: 11%; }
.col-identby   { width: 7%; }
.col-dateident { width: 6%; text-align: center; }
.col-desc      { width: 13%; }
.col-assigned  { width: 7%; }
.col-priority  { width: 5%; text-align: center; }
.col-status    { width: 6%; text-align: center; }
.col-duedate   { width: 6%; text-align: center; }
.col-lastupd   { width: 6%; text-align: center; }
.col-resolved  { width: 6%; text-align: center; }
.col-comments  { width: 21%; }
.col-int       { width: 3%; text-align: center; }

.row-tracker   { background-color: #DCE6F1; }
.row-action    { background-color: #DCE6F1; }

.priority-high   { color: #C00000; font-weight: bold; }
.priority-medium { color: #9C6500; font-weight: bold; }
.priority-low    { color: #375623; font-weight: bold; }

.group-header {
    page-break-after: avoid;
    break-after: avoid;
}
.group-header td {
    background-color: #1F497D;
    color: #FFFFFF;
    border: 1px solid #808080;
    padding: 3px 4px;
    font-size: 8pt;
    font-weight: bold;
}

.footer {
    margin-top: 8px;
    font-size: 7.5pt;
    color: #666;
}
.branding {
    font-size: 9pt;
    color: #000000;
    margin-top: 2px;
}
.branding a {
    color: #0066CC;
    text-decoration: none;
}
.branding a:hover {
    text-decoration: underline;
}
</style></head><body>)HTML") + fragment + "</body></html>";
 QTextDocument plainDocument; plainDocument.setHtml(fragment); r.plainText=plainDocument.toPlainText();r.defaultSubject=in.projectNumber+" "+in.projectName+" - Tracker Items "+date;r.fileStem=in.projectNumber+" Tracker Items"+(in.options.internalReport?" Internal":"");r.pdfLayout=QPageLayout(QPageSize(QPageSize::Letter),QPageLayout::Landscape,QMarginsF(12,12,12,12),QPageLayout::Millimeter);return r;
}
}

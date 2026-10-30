// SSRS status and tracker reports: downloads the report as PDF (and Word when
// needed), files it in the project folder, and prepares the email. Replaces
// plugins/ifs_ssrs_generate_plugin.py.
import * as pn from "projectnotes";
import { replaceVariables, formatDate } from "projectnotes/common";

import { trace } from "./sso.mjs";

const { app, data, email, files, http, log, secrets, settings, ui } = pn;

// What differs between the reports. The status report also records the
// project's last status date.
const REPORTS = {
    status: { title: "Status Report", workflow: "status-report", folderKey: "SsrsExportSubFolder",
              folder: "Project Management/Status Reports", recordsStatusDate: true },
    tracker: { title: "Tracker Report", workflow: "tracker-items-report", folderKey: "TrackerExportSubFolder",
               folder: "Project Management/Issues List", recordsStatusDate: false },
};

const DELIVERY = { html: "Email the report in the message", pdf: "Email the PDF", word: "Email the Word document", none: "Do not email" };

function today() {
    const d = new Date();
    const pad = n => (n < 10 ? "0" : "") + n;
    return d.getFullYear() + "-" + pad(d.getMonth() + 1) + "-" + pad(d.getDate());
}

// The report period parameter takes days.
function reportUrl(template, project, reportDate) {
    let url = replaceVariables(template, project, { "report.date": formatDate(reportDate, "MM/dd/yyyy") });
    url = url.replace("&ReportPeriod=Bi-Weekly", "&ReportPeriod=14").replace("&ReportPeriod=Weekly", "&ReportPeriod=7")
             .replace("&ReportPeriod=Monthly", "&ReportPeriod=30").replace("&ReportPeriod=None", "&ReportPeriod=30")
             .replace("&ReportPeriod=&", "&ReportPeriod=30&");
    if (/&ReportPeriod=$/.test(url)) url += "30";
    return url;
}

function* download(url, path) {
    const user = settings.get("DomainUser", "") || "";
    const password = (yield secrets.get("DomainPassword")) || "";
    trace("ssrs", "downloading " + url + " as " + (user || "(no user)") + ", password " + (password ? "set" : "MISSING"));
    const response = yield http.fetch(url, {
        auth: { type: "basic", username: user, password: password },
        timeoutSeconds: 300,
    });
    const headers = response.headers || {};
    const type = Object.keys(headers).filter(k => k.toLowerCase() === "content-type").map(k => headers[k])[0] || "";
    trace("ssrs", "report server answered " + response.status + " " + type);
    if (!response.ok) {
        yield ui.message("The report server answered " + response.status + ".\n\n" + (yield response.text()).slice(0, 500),
                         { kind: "error", title: "Download Failed" });
        return false;
    }
    const bytes = yield response.arrayBuffer();
    yield files.writeBytes(path, bytes);
    trace("ssrs", "wrote " + bytes.byteLength + " bytes to " + path);
    return true;
}

// Word's filtered HTML version of the report, for the message body (Windows).
function* wordToHtml(docxPath, title) {
    const htmlPath = files.tempPath("ssrs-report.html");
    let word = null;
    try {
        trace("ssrs", "converting " + docxPath + " to HTML with Word");
        word = pn.com.createObject("Word.Application");
        word.Visible = false;
        const doc = word.Documents.Open(docxPath.replace(/\//g, "\\"));
        doc.SpellingChecked = false;
        doc.GrammarChecked = false;
        doc.SaveAs(htmlPath.replace(/\//g, "\\"), 8);   // wdFormatHTML
        doc.Close(false);
        word.Quit();
        trace("ssrs", "Word converted the report");
    } catch (e) {
        trace("ssrs", "Word conversion failed: " + e.message);
        // Don't leave a hidden Word running in the background.
        if (word) {
            try { word.Quit(false); } catch (quitError) { /* already gone */ }
        }
        yield ui.message("Word could not convert the report: " + e.message, { kind: "error", title: "Generate " + title });
        return null;
    } finally {
        if (word) pn.com.release(word);
    }
    const html = yield files.readText(htmlPath);
    yield files.remove(htmlPath);
    return html;
}

// The project's default audience for the report, or the team members marked
// to receive status reports when the project has none.
function* recipients(project, workflow) {
    try {
        const audiences = yield email.audiences(project.$key, workflow);
        const chosen = audiences.filter(a => a.isDefault)[0] || audiences[0];
        if (chosen) return yield email.resolveAudience(project.$key, chosen.id);
    } catch (e) {
        log.warn("Could not read the report audience: " + e.message);
    }
    const defaults = yield app.defaults();
    const to = (project.project_team || [])
        .filter(m => m.email && (m.receive_status_report === true || m.receive_status_report === 1 || m.receive_status_report === "1"))
        .filter(m => !defaults.projectManagerId || !m.contact_id || m.contact_id.id !== defaults.projectManagerId)
        .map(m => ({ name: m.contact_id ? m.contact_id.lookup : "", email: m.email }));
    return { to: to, cc: [], bcc: [] };
}

export function* generateStatusReport(context) {
    yield generateReport(context, REPORTS.status);
}

export function* generateTrackerReport(context) {
    yield generateReport(context, REPORTS.tracker);
}

function* generateReport(context, report) {
    const project = context.record;
    const server = String(settings.get("ReportServer", "") || "").trim();
    trace("ssrs", report.title + ": project " + (project ? project.project_number + " (" + project.$key + ")" : "MISSING")
          + ", report server " + (server || "(none)") + ", Word available " + !!pn.com);
    if (!server) {
        yield ui.message("Enter the report server in Settings > IFS Cloud first.", { kind: "warning", title: "Report Server Not Configured" });
        return;
    }
    const canConvert = !!pn.com;
    const choices = [DELIVERY.pdf, DELIVERY.word, DELIVERY.none];
    if (canConvert) choices.unshift(DELIVERY.html);
    trace("ssrs", "showing the report options form");
    const options = yield ui.showForm({ fields: [
        { key: "reportDate", label: "Report date", type: "date" },
        { key: "delivery", label: "Email", type: "choice", choices: choices },
        { key: "keepWord", label: "Save a Word copy in the project folder", type: "checkbox" },
        { key: "display", label: "Open the PDF when it is ready", type: "checkbox" },
    ] }, { reportDate: today(), delivery: choices[0], keepWord: false, display: false },
       { title: "Generate " + report.title, okText: "Generate" });
    trace("ssrs", "form answered " + JSON.stringify(options));
    if (!options) return;

    const reportDate = options.reportDate || today();
    const delivery = options.delivery;
    const stamp = reportDate.replace(/-/g, "") + " " + project.project_number + " " + report.title;
    const pdfPath = files.tempPath(stamp + ".pdf");
    const docxPath = files.tempPath(stamp + ".docx");
    const needWord = delivery === DELIVERY.html || delivery === DELIVERY.word || options.keepWord;
    const base = "http://" + server + reportUrl(context.parameter, project, reportDate);

    const ok = yield ui.progress("Generating the " + report.title.toLowerCase(), function* (progress) {
        if (needWord) {
            progress.update(0.2, "Downloading the Word report...");
            if (!(yield download(base + "&rs:Command=Render&rs:Format=WORDOPENXML", docxPath))) return false;
        }
        progress.update(0.5, "Downloading the PDF report...");
        return yield download(base + "&rs:Command=Render&rs:Format=PDF", pdfPath);
    }, { cancellable: false });
    trace("ssrs", "downloads " + (ok ? "succeeded" : "failed"));
    if (!ok) return;

    let folder = yield app.projectFolder(project.$key);
    trace("ssrs", "project folder " + (folder || "(none)"));
    if (folder) {
        const sub = String(settings.get(report.folderKey, "") || report.folder).replace(/^[\\/]+|[\\/]+$/g, "");
        folder = folder.replace(/[\\/]+$/, "") + "/" + sub;
        if (!(yield files.exists(folder))) {
            log.info("The " + report.title.toLowerCase() + " folder " + folder + " does not exist; the report stays in the temporary folder.");
            folder = "";
        }
    }
    let shown = pdfPath;
    if (folder) {
        shown = folder + "/" + stamp + ".pdf";
        yield files.copy(pdfPath, shown, { overwrite: true });
        if (options.keepWord) yield files.copy(docxPath, folder + "/" + stamp + ".docx", { overwrite: true });
        trace("ssrs", "filed the report as " + shown);
    }

    if (delivery !== DELIVERY.none) {
        const subject = project.project_number + " " + (project.project_name || "") + " " + report.title + " " + formatDate(reportDate, "MM/dd/yyyy");
        const message = Object.assign({ subject: subject, projectKey: project.$key }, yield recipients(project, report.workflow));
        if (delivery === DELIVERY.html) {
            const html = yield wordToHtml(docxPath, report.title);
            if (html) message.html = html;
            else message.attachments = [pdfPath];
        } else {
            message.attachments = [delivery === DELIVERY.word ? docxPath : pdfPath];
            message.text = "";
        }
        trace("ssrs", "composing email: to " + (message.to || []).length + ", cc " + (message.cc || []).length
              + ", " + (message.html ? "HTML body " + message.html.length + " chars" : "attachments " + JSON.stringify(message.attachments)));
        const composed = yield email.compose(message);
        trace("ssrs", "email compose returned " + JSON.stringify(composed));
    }

    if (options.display) yield files.openWithSystem(shown);
    if (report.recordsStatusDate) yield data.save("projects", { $key: project.$key, last_status_date: reportDate });
    ui.notify("Saved " + stamp + ".pdf", { kind: "success" });
}

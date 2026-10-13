// SSRS status report: downloads the report as PDF (and Word when needed),
// files it in the project folder, and prepares the status email. Replaces
// plugins/ifs_ssrs_generate_plugin.py.
import * as pn from "projectnotes";
import { replaceVariables, formatDate } from "projectnotes/common";

const { app, data, email, files, http, log, secrets, settings, ui } = pn;

const DELIVERY = { html: "Email the report in the message", pdf: "Email the PDF", word: "Email the Word document", none: "Do not email" };

function today() {
    const d = new Date();
    const pad = n => (n < 10 ? "0" : "") + n;
    return d.getFullYear() + "-" + pad(d.getMonth() + 1) + "-" + pad(d.getDate());
}

// The report period parameter takes days.
function reportUrl(template, project, reportDate) {
    let url = replaceVariables(template, project, { REPORTDATE: formatDate(reportDate, "MM/dd/yyyy") });
    url = url.replace("&ReportPeriod=Bi-Weekly", "&ReportPeriod=14").replace("&ReportPeriod=Weekly", "&ReportPeriod=7")
             .replace("&ReportPeriod=Monthly", "&ReportPeriod=30").replace("&ReportPeriod=None", "&ReportPeriod=30")
             .replace("&ReportPeriod=&", "&ReportPeriod=30&");
    if (/&ReportPeriod=$/.test(url)) url += "30";
    return url;
}

function* download(url, path) {
    const response = yield http.fetch(url, {
        auth: { type: "basic", username: settings.get("DomainUser", "") || "", password: (yield secrets.get("DomainPassword")) || "" },
        timeoutSeconds: 300,
    });
    if (!response.ok) {
        yield ui.message("The report server answered " + response.status + ".\n\n" + (yield response.text()).slice(0, 500),
                         { kind: "error", title: "Download Failed" });
        return false;
    }
    yield files.writeBytes(path, yield response.arrayBuffer());
    return true;
}

// Word's filtered HTML version of the report, for the message body (Windows).
function* wordToHtml(docxPath) {
    const htmlPath = files.tempPath("status-report.html");
    let word = null;
    try {
        word = pn.com.createObject("Word.Application");
        word.Visible = false;
        const doc = word.Documents.Open(docxPath.replace(/\//g, "\\"));
        doc.SpellingChecked = false;
        doc.GrammarChecked = false;
        doc.SaveAs(htmlPath.replace(/\//g, "\\"), 8);   // wdFormatHTML
        doc.Close(false);
        word.Quit();
    } catch (e) {
        yield ui.message("Word could not convert the report: " + e.message, { kind: "error", title: "Generate Status Report" });
        return null;
    } finally {
        if (word) pn.com.release(word);
    }
    const html = yield files.readText(htmlPath);
    yield files.remove(htmlPath);
    return html;
}

// The project's status report audience, or the team members marked to
// receive status reports when the project has none.
function* recipients(project) {
    try {
        const audiences = yield email.audiences(project.$key, "status-report");
        const chosen = audiences.filter(a => a.isDefault)[0] || audiences[0];
        if (chosen) return yield email.resolveAudience(project.$key, chosen.id);
    } catch (e) {
        log.warn("Could not read the status report audience: " + e.message);
    }
    const defaults = yield app.defaults();
    const to = (project.project_team || [])
        .filter(m => m.email && (m.receive_status_report === true || m.receive_status_report === 1 || m.receive_status_report === "1"))
        .filter(m => !defaults.projectManagerId || !m.contact_id || m.contact_id.id !== defaults.projectManagerId)
        .map(m => ({ name: m.contact_id ? m.contact_id.lookup : "", email: m.email }));
    return { to: to, cc: [], bcc: [] };
}

export function* generateStatusReport(context) {
    const project = context.record;
    const server = String(settings.get("ReportServer", "") || "").trim();
    if (!server) {
        yield ui.message("Enter the report server in Settings > IFS Cloud first.", { kind: "warning", title: "Report Server Not Configured" });
        return;
    }
    const canConvert = !!pn.com;
    const choices = [DELIVERY.pdf, DELIVERY.word, DELIVERY.none];
    if (canConvert) choices.unshift(DELIVERY.html);
    const options = yield ui.showForm({ fields: [
        { key: "reportDate", label: "Report date", type: "date" },
        { key: "delivery", label: "Email", type: "choice", choices: choices },
        { key: "keepWord", label: "Save a Word copy in the project folder", type: "checkbox" },
        { key: "display", label: "Open the PDF when it is ready", type: "checkbox" },
    ] }, { reportDate: today(), delivery: choices[0], keepWord: false, display: false },
       { title: "Generate Status Report", okText: "Generate" });
    if (!options) return;

    const reportDate = options.reportDate || today();
    const delivery = options.delivery;
    const stamp = reportDate.replace(/-/g, "") + " " + project.project_number + " Status Report";
    const pdfPath = files.tempPath(stamp + ".pdf");
    const docxPath = files.tempPath(stamp + ".docx");
    const needWord = delivery === DELIVERY.html || delivery === DELIVERY.word || options.keepWord;
    const base = "http://" + server + reportUrl(context.parameter, project, reportDate);

    const ok = yield ui.progress("Generating the status report", function* (progress) {
        if (needWord) {
            progress.update(0.2, "Downloading the Word report...");
            if (!(yield download(base + "&rs:Command=Render&rs:Format=WORDOPENXML", docxPath))) return false;
        }
        progress.update(0.5, "Downloading the PDF report...");
        return yield download(base + "&rs:Command=Render&rs:Format=PDF", pdfPath);
    }, { cancellable: false });
    if (!ok) return;

    let folder = yield app.projectFolder(project.$key);
    if (folder) {
        const sub = String(settings.get("SsrsExportSubFolder", "") || "Project Management/Status Reports").replace(/^[\\/]+|[\\/]+$/g, "");
        folder = folder.replace(/[\\/]+$/, "") + "/" + sub;
        if (!(yield files.exists(folder))) {
            log.info("The status report folder " + folder + " does not exist; the report stays in the temporary folder.");
            folder = "";
        }
    }
    let shown = pdfPath;
    if (folder) {
        shown = folder + "/" + stamp + ".pdf";
        yield files.copy(pdfPath, shown, { overwrite: true });
        if (options.keepWord) yield files.copy(docxPath, folder + "/" + stamp + ".docx", { overwrite: true });
    }

    if (delivery !== DELIVERY.none) {
        const subject = project.project_number + " " + (project.project_name || "") + " Status Report " + formatDate(reportDate, "MM/dd/yyyy");
        const message = Object.assign({ subject: subject, projectKey: project.$key }, yield recipients(project));
        if (delivery === DELIVERY.html) {
            const html = yield wordToHtml(docxPath);
            if (html) message.html = html;
            else message.attachments = [pdfPath];
        } else {
            message.attachments = [delivery === DELIVERY.word ? docxPath : pdfPath];
            message.text = "";
        }
        yield email.compose(message);
    }

    if (options.display) yield files.openWithSystem(shown);
    yield data.save("projects", { $key: project.$key, last_status_date: reportDate });
    ui.notify("Saved " + stamp + ".pdf", { kind: "success" });
}

// Office Tools (Windows): project documents from Office templates, Open MS
// Project, and cleanup of stranded Excel and Word automation processes.
// Replaces newchangeorder_plugin.py, newpcrregistry_plugin.py,
// newpowerpoint_plugin.py, newriskregister_plugin.py, newmsproject_plugin.py,
// open_msproject_plugin.py, excelkill_plugin.py and wordkill_plugin.py.
import * as pn from "projectnotes";

// com exists only where the build provides it, so read it from the namespace.
const { app, data, files, settings, ui } = pn;

const DEFAULT_FOLDERS = {
    ChangeOrder: "Project Management/PCR's",
    PcrRegister: "Project Management/PCR's",
    PowerPoint: "Project Management/Meeting Minutes",
    RiskRegister: "Project Management/Risk Management",
    MsProject: "Project Management/Schedule",
};

function text(value) {
    if (value === null || value === undefined) return "";
    if (typeof value === "object") return value.lookup || "";
    return String(value);
}

// Office, and the paths Project Notes stores, use backslashes; files.* takes
// either on Windows.
function windowsPath(path) { return String(path).replace(/\//g, "\\"); }

function fileUrl(path) { return "file:///" + String(path).replace(/\\/g, "/").replace(/^\/+/, ""); }

function baseName(path) { return String(path).replace(/^.*[\\/]/, "").replace(/\.[^.]*$/, ""); }
function suffix(path) { const m = /\.([^.\\/]*)$/.exec(String(path)); return m ? m[1] : ""; }

// The project's folder plus the configured subfolder, or a folder the user
// picks when the project has none.
function* outputFolder(context, prefix) {
    const projectFolder = yield app.projectFolder(context.key);
    if (projectFolder && (yield files.exists(projectFolder))) {
        const sub = String(settings.get(prefix + "ExportSubFolder", "") || DEFAULT_FOLDERS[prefix]).replace(/^[\\/]+|[\\/]+$/g, "");
        return projectFolder.replace(/[\\/]+$/, "") + (sub ? "/" + sub : "");
    }
    return yield ui.pickFolder({ title: "Select an output folder" });
}

// A template shipped in this plugin's templates folder, or one the user picks
// when it is not installed.
function* bundledTemplate(fileName, filters) {
    const path = files.packagePath("templates/" + fileName);
    if (yield files.exists(path)) return path;
    return yield ui.pickFile({ title: "Select the " + baseName(fileName), filters: filters });
}

// Copy the template unless the target already exists. Returns false when the
// copy failed.
function* copyTemplate(template, target) {
    if (yield files.exists(target)) return true;
    try {
        yield files.copy(template, target);
        return true;
    } catch (e) {
        yield ui.message("Could not copy " + template + " to " + target + ".\n\n" + e.message,
                         { kind: "error", title: "Unable to copy template" });
        return false;
    }
}

function* addLocation(context, type, description, path) {
    yield data.save("project_locations", {
        project_id: { id: context.key },
        location_type: type,
        location_description: description,
        full_path: windowsPath(path),
    });
}

// "<project number> <template name without ' Template'><extra>.<ext>"
function documentName(project, template, extra) {
    return (text(project.project_number) + " " + baseName(template) + (extra || "") + "." + suffix(template))
        .replace(" Template", "");
}

function* withOffice(progId, work) {
    let application = null;
    try {
        application = pn.com.createObject(progId);
        return yield* work(application);
    } catch (e) {
        yield ui.message(e.message, { kind: "error", title: progId });
        return null;
    } finally {
        if (application) pn.com.release(application);
    }
}

// Template placeholders use the same {{ dotted.path }} tokens as the email
// templates. Each entry lists every spelling to find: the token with and
// without inner spaces, and the <TAG> older templates were written with.
function placeholder(path, legacyTag, value) {
    const find = ["{{ " + path + " }}", "{{" + path + "}}"];
    if (legacyTag) find.push("<" + legacyTag + ">");
    return { find: find, value: String(value) };
}

function projectPlaceholders(project) {
    return [
        placeholder("project.number", "PROJECTNUMBER", text(project.project_number)),
        placeholder("project.name", "PROJECTNAME", text(project.project_name)),
        placeholder("company.name", "CLIENTNAME", text(project.company_id)),
    ];
}

function today() {
    const now = new Date();
    const pad = n => (n < 10 ? "0" : "") + n;
    return pad(now.getMonth() + 1) + "/" + pad(now.getDate()) + "/" + now.getFullYear();
}

function wordReplace(doc, search, replacement) {
    // Find.Execute(FindText, MatchCase, MatchWholeWord, MatchWildcards,
    // MatchSoundsLike, MatchAllWordForms, Forward, Wrap=wdFindContinue,
    // Format, ReplaceWith, Replace=wdReplaceAll)
    return doc.Content.Find.Execute(search, false, false, false, false, false, true, 1, false, replacement, 2);
}

function* changeOrder(context) {
    const project = context.record;
    const number = yield ui.prompt("Number 0#:", { title: "Change Order Number" });
    if (number === null) return;
    const folder = yield outputFolder(context, "ChangeOrder");
    if (!folder) return;
    const template = yield bundledTemplate("PCR Template.docx", ["Word documents (*.docx *.doc)"]);
    if (!template) return;
    const name = documentName(project, template, number);
    const target = folder + "/" + name;
    if (!(yield copyTemplate(template, target))) return;

    const entries = projectPlaceholders(project).concat([
        placeholder("current.date", "CRDATE", today()),
        placeholder("change.number", "CRNUMBER", text(project.project_number).replace(/\D/g, "") + "-" + number),
    ]);
    const done = yield withOffice("Word.Application", function* (word) {
        const doc = word.Documents.Open(windowsPath(target));
        for (const entry of entries)
            for (const find of entry.find) wordReplace(doc, find, entry.value);
        doc.Save();
        word.Visible = true;
        doc.Activate();
        word.Activate();
        return true;
    });
    if (done) yield addLocation(context, "Word Document", "Change Request : " + name, target);
}

function* pcrRegister(context) {
    const folder = yield outputFolder(context, "PcrRegister");
    if (!folder) return;
    const template = yield bundledTemplate("PCR Registry Template.xlsx", ["Excel workbooks (*.xlsx *.xlsm *.xls)"]);
    if (!template) return;
    const name = documentName(context.record, template);
    const target = folder + "/" + name;
    if (!(yield copyTemplate(template, target))) return;
    yield ui.openUrl(fileUrl(target));
    yield addLocation(context, "Excel Document", name, target);
}

function* riskRegister(context) {
    const project = context.record;
    const folder = yield outputFolder(context, "RiskRegister");
    if (!folder) return;
    const template = yield bundledTemplate("Risk Register Template.xlsx", ["Excel workbooks (*.xlsx *.xlsm *.xls)"]);
    if (!template) return;
    const name = documentName(project, template);
    const target = folder + "/" + name;
    if (!(yield copyTemplate(template, target))) return;

    const done = yield withOffice("Excel.Application", function* (excel) {
        excel.Visible = false;
        excel.ScreenUpdating = false;
        excel.EnableEvents = false;
        excel.DisplayAlerts = false;
        const workbook = excel.Workbooks.Open(windowsPath(target));
        try {
            const sheet = workbook.Sheets("Risk Register");
            // Replace(What, Replacement, LookAt=xlPart). The older <PROJECTNAME>
            // tag also carried the project number in front.
            const entries = [
                placeholder("project.number", null, text(project.project_number)),
                placeholder("project.name", null, text(project.project_name)),
                placeholder("company.name", "CLIENTNAME", text(project.company_id)),
                placeholder("current.date", null, today()),
                { find: ["<PROJECTNAME>"], value: text(project.project_number) + " " + text(project.project_name) },
            ];
            for (const entry of entries)
                for (const find of entry.find)
                    sheet.Range("A1", "Z10000").Replace(find, entry.value.slice(0, 255), 2);
            workbook.Save();
        } finally {
            excel.Workbooks.Close();
            excel.Quit();
        }
        return true;
    });
    if (!done) return;
    yield ui.openUrl(fileUrl(target));
    yield addLocation(context, "Excel Document", name, target);
}

function* powerPoint(context) {
    const project = context.record;
    const folder = yield outputFolder(context, "PowerPoint");
    if (!folder) return;
    const template = yield ui.pickFile({ title: "Select the PowerPoint Template",
                                         startIn: files.packagePath("templates"),
                                         filters: ["PowerPoint files (*.ppt *.pptx *.pptm)"] });
    if (!template) return;
    const name = documentName(project, template);
    const target = folder + "/" + name;
    if (!(yield copyTemplate(template, target))) return;

    const done = yield withOffice("PowerPoint.Application", function* (powerpoint) {
        const presentation = powerpoint.Presentations.Open(windowsPath(target));
        const count = presentation.Slides.Count;
        const entries = projectPlaceholders(project).concat([placeholder("current.date", null, today())]);
        for (let i = 1; i <= count; ++i) {
            for (const shape of presentation.Slides(i).Shapes) {
                if (shape.HasTextFrame && shape.TextFrame.HasText) {
                    for (const entry of entries)
                        for (const find of entry.find) shape.TextFrame.TextRange.Replace(find, entry.value);
                }
            }
        }
        presentation.Save();
        return true;
    });
    if (done) yield addLocation(context, "PowerPoint Document", name, target);
}

function* msProject(context) {
    const project = context.record;
    const folder = yield outputFolder(context, "MsProject");
    if (!folder) return;
    const template = yield ui.pickFile({ title: "Select the MS Project Template",
                                         startIn: files.packagePath("templates"),
                                         filters: ["Project files (*.mpp)"] });
    if (!template) return;
    const name = text(project.project_name).slice(0, 30);
    const target = folder + "/" + name + ".mpp";
    if (!(yield copyTemplate(template, target))) return;

    const done = yield withOffice("MSProject.Application", function* (msp) {
        msp.FileOpen(windowsPath(target));
        msp.Visible = true;
        // ReplaceEx(Field, Test, Value, Replacement, ReplaceAll, Next,
        // MatchCase, FieldType, Replacement field type, Search all fields)
        // The older bare PROJECTNAME text and the {{ project.name }} token both work.
        for (const find of ["PROJECTNAME", "{{ project.name }}", "{{project.name}}"])
            msp.ReplaceEx("Name", "contains", find, name, true, true, false, 188743694, 7, true);
        msp.FileSave();
        return true;
    });
    if (done) yield addLocation(context, "Microsoft Project", "Project Schedule : " + name + ".mpp", target);
}

function* openMsProject(context) {
    // Asking for the project folder lets the plugin check files inside it.
    yield app.projectFolder(context.key);
    const schedules = [];
    for (const location of context.record.project_locations || []) {
        const path = text(location.full_path);
        if (!/\.mpp$/i.test(path)) continue;
        let exists = true;
        try { exists = yield files.exists(path); } catch (e) { /* outside the project folder; let Windows decide */ }
        if (exists) schedules.push(path);
    }
    if (!schedules.length) {
        yield ui.message("No MS Project files were found. Check that they are listed in Files & Folders and that they are accessible.",
                         { kind: "error", title: "No MS Project File Found" });
        return;
    }
    const chosen = schedules.length === 1 ? schedules[0]
        : yield ui.choose("Select the schedule to open:", schedules, { title: "Open MS Project" });
    if (chosen) yield ui.openUrl(fileUrl(chosen));
}

// Office started through automation runs with /automation on its command line.
function* killAutomation(exe, product) {
    let closed = 0;
    let wmi = null;
    try {
        wmi = pn.com.getObject("winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\cimv2");
        const processes = wmi.ExecQuery("Select * from Win32_Process Where Name = '" + exe + "' and CommandLine like '%automation%'");
        for (const process of processes) {
            // $call invokes directly; WMI objects may run a method on a plain property read.
            try { process.$call("Terminate"); ++closed; } catch (e) { /* already gone */ }
        }
    } catch (e) {
        yield ui.message("Could not look for stranded " + product + " processes: " + e.message,
                         { kind: "error", title: "Close Stranded " + product });
        return;
    } finally {
        if (wmi) pn.com.release(wmi);
    }
    if (closed > 0)
        yield ui.message("Closed " + closed + " stranded Microsoft " + product + " process(es) running in the background.",
                         { title: "Closed Stranded " + product });
    else
        yield ui.message("No stranded Microsoft " + product + " processes were found.",
                         { title: "No Stranded " + product + " Found" });
}

const handlers = {
    "change-order": changeOrder,
    "pcr-register": pcrRegister,
    "powerpoint": powerPoint,
    "risk-register": riskRegister,
    "ms-project": msProject,
    "open-ms-project": openMsProject,
    "kill-excel": function* () { yield* killAutomation("EXCEL.EXE", "Excel"); },
    "kill-word": function* () { yield* killAutomation("WINWORD.EXE", "Word"); },
};

export function* onMenu(menuId, context) {
    const handler = handlers[menuId];
    if (handler) yield* handler(context);
}

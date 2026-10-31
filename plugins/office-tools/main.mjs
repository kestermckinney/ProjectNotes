// MS Office Templates (Windows): project documents from the Word, Excel,
// PowerPoint and MS Project templates configured in Settings > MS Office Templates, Open MS Project,
// and cleanup of stranded Excel and Word automation processes.
import * as pn from "projectnotes";
import { render, validFilename } from "projectnotes/common";

// com exists only where the build provides it, so read it from the namespace.
const { app, data, files, menus, settings, ui } = pn;

// The templates configured on first start. A relative template path is
// inside this plugin's folder.
const DEFAULT_TEMPLATES = [
    { menu: "Change Request", submenu: "Templates", entity: "projects",
      template: "templates/Change Request Template.docx",
      folder: "Project Management/PCR's",
      filename: "{{ projects.project_number }} Change Request {{ input.change_number }}" },
    { menu: "Tracker Items Status", submenu: "Templates", entity: "projects",
      template: "templates/Tracker Items Status Template.xlsx",
      folder: "Project Management/Issues",
      filename: "{{ projects.project_number }} Tracker Items {{ current.isoDate }}" },
    { menu: "Project Kickoff", submenu: "Templates", entity: "projects",
      template: "templates/Project Kickoff Template.pptx",
      folder: "Project Management/Meeting Minutes",
      filename: "{{ projects.project_number }} Project Kickoff" },
    { menu: "Project Schedule", submenu: "Templates", entity: "projects",
      template: "templates/Software Development Lifecycle Template.xml",
      folder: "Project Management/Schedule",
      filename: "{{ projects.project_number }} Schedule" },
];

// What each template type opens with, the Office SaveAs format of the copy,
// and the extension a template file (.dotx and so on) is saved under.
const KINDS = {
    docx: { office: "word", format: 16 }, docm: { office: "word", format: 13 }, doc: { office: "word", format: 0 },
    dotx: { office: "word", format: 16, ext: "docx" }, dotm: { office: "word", format: 13, ext: "docm" },
    dot: { office: "word", format: 0, ext: "doc" },
    xlsx: { office: "excel", format: 51 }, xlsm: { office: "excel", format: 52 }, xls: { office: "excel", format: 56 },
    xltx: { office: "excel", format: 51, ext: "xlsx" }, xltm: { office: "excel", format: 52, ext: "xlsm" },
    pptx: { office: "powerpoint", format: 24 }, pptm: { office: "powerpoint", format: 25 }, ppt: { office: "powerpoint", format: 1 },
    potx: { office: "powerpoint", format: 24, ext: "pptx" }, potm: { office: "powerpoint", format: 25, ext: "pptm" },
    // MS Project saves every schedule as .mpp (pjMPP); .xml is Project's XML format.
    mpp: { office: "project", format: 0 }, mpt: { office: "project", format: 0, ext: "mpp" },
    xml: { office: "project", format: 0, ext: "mpp" },
};
const OFFICE = {
    word: { progId: "Word.Application", locationType: "Word Document", open: wordDocument },
    excel: { progId: "Excel.Application", locationType: "Excel Document", open: excelDocument },
    powerpoint: { progId: "PowerPoint.Application", locationType: "PowerPoint Document", open: powerPointDocument },
    project: { progId: "MSProject.Application", locationType: "Microsoft Project", open: projectDocument,
               needs: "Microsoft Project" },
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
function suffix(path) { const m = /\.([^.\\/]*)$/.exec(String(path)); return m ? m[1].toLowerCase() : ""; }
function isAbsolute(path) { return /^(?:[A-Za-z]:[\\/]|[\\/]{2}|\/)/.test(path); }

function pad(n) { return (n < 10 ? "0" : "") + n; }
function usDate(iso) { return iso.slice(5, 7) + "/" + iso.slice(8, 10) + "/" + iso.slice(0, 4); }

// ── Settings and menus ─────────────────────────────────────────────────────

function templateRows() {
    const rows = settings.get("templates", []) || [];
    return Array.prototype.slice.call(rows);
}

function register() {
    menus.clear();
    const items = [];
    templateRows().forEach(function (row, i) {
        if (!row) return;
        const title = String(row.menu || "").trim();
        const entity = String(row.entity || "").trim();
        if (!title || !entity || !String(row.template || "").trim()) return;
        items.push({ id: "template-" + i, title: title, submenu: String(row.submenu || "").trim(),
                     context: "record", entity: entity, parameter: String(i), handler: createDocument });
    });
    if (items.length) menus.register(items);
}

export function onStart() {
    if (settings.get("templates") === null) settings.set("templates", DEFAULT_TEMPLATES);
    register();
}

export function onSettingsChanged() {
    register();
}

// ── Values ─────────────────────────────────────────────────────────────────

function money(n) {
    const parts = Math.abs(n).toFixed(2).split(".");
    return (n < 0 ? "-$" : "$") + parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ",") + "." + parts[1];
}

// How a record value reads in a document: references by their display value,
// dates as MM/dd/yyyy, checkboxes as Yes or No, currency as $1,234.00.
function display(value, type) {
    if (value === null || value === undefined) return "";
    if (typeof value === "number" && type === "currency") return money(value);
    if (typeof value === "number" && type === "percent") return value + "%";
    if (typeof value === "object") return value.lookup !== null && value.lookup !== undefined ? String(value.lookup) : String(value.id || "");
    if (typeof value === "boolean") return value ? "Yes" : "No";
    const s = String(value);
    return /^\d{4}-\d{2}-\d{2}(?:T[\d:]+)?$/.test(s) ? usDate(s) : s;
}

function compareItems(a, b) {
    const x = text(a.item_number), y = text(b.item_number);
    const nx = Number(x), ny = Number(y);
    if (x !== "" && y !== "" && !isNaN(nx) && !isNaN(ny)) return nx - ny;
    return x < y ? -1 : x > y ? 1 : 0;
}

// The record with all of its related lists.
function* loadRecord(entity, key) {
    const schema = yield data.schema(entity);
    const include = (schema.children || []).map(function (c) { return c.entity; });
    const record = yield data.get(entity, key, include.length ? { include: include } : {});
    if (record && Array.isArray(record.item_tracker)) record.item_tracker.sort(compareItems);
    return { record: record, schema: schema };
}

// The project a record belongs to: the record itself, the project it refers
// to, or the project of the tracker item or meeting it refers to.
function* projectOf(entity, record, schema) {
    if (entity === "projects") return record;
    const refs = (schema.columns || []).filter(function (c) { return c.type === "ref" && c.ref; });
    let projectKey = null;
    for (const c of refs) {
        const ref = record[c.name];
        if (c.ref.entity === "projects" && ref && ref.id) { projectKey = ref.id; break; }
    }
    if (!projectKey) {
        for (const c of refs) {
            const ref = record[c.name];
            if (!ref || !ref.id || (c.ref.entity !== "item_tracker" && c.ref.entity !== "project_notes")) continue;
            const parent = yield data.get(c.ref.entity, ref.id);
            if (parent && parent.project_id && parent.project_id.id) { projectKey = parent.project_id.id; break; }
        }
    }
    return projectKey ? (yield loadRecord("projects", projectKey)).record : null;
}

// Placeholders, the same {{ dotted.path }} grammar as My Shortcuts:
//   entity.column      the record right-clicked, its project, or the first row of a related list
//   entity.N.column    row N of a related list
//   entity.*.column    that column of every row, joined with ", "
//   entity.#.column    in a table row: the row repeats once for each row of the list
//   input.name         asked for when the document is created
// plus the names in `extras`. A related list is the clicked record's own, or
// else its project's, so tracker items list the project's other items.
function makeSource(root, project, types, extras) {
    function show(record, column) {
        const columns = types[record.$entity];
        return display(record[column], columns ? columns[column] : undefined);
    }
    function rows(entity) {
        if (root && Array.isArray(root[entity])) return root[entity];
        if (project && Array.isArray(project[entity])) return project[entity];
        if (root && root.$entity === entity) return [root];
        if (project && project.$entity === entity) return [project];
        return [];
    }
    function single(entity) {
        if (root && root.$entity === entity) return root;
        if (project && project.$entity === entity) return project;
        return rows(entity)[0] || null;
    }
    function lookup(path) {
        if (Object.prototype.hasOwnProperty.call(extras, path)) return extras[path];
        const parts = path.split(".");
        if (parts.length === 2) {
            const record = single(parts[0]);
            return record ? show(record, parts[1]) : undefined;
        }
        if (parts.length === 3) {
            const list = rows(parts[0]);
            if (parts[1] === "*")
                return list.map(function (r) { return show(r, parts[2]); }).filter(function (v) { return v !== ""; }).join(", ");
            if (/^\d+$/.test(parts[1])) {
                const record = list[+parts[1] - 1];
                return record ? show(record, parts[2]) : undefined;
            }
        }
        return undefined;
    }
    return {
        extras: extras,
        rows: rows,
        // `row` (1-based) stands in for "#".
        fill: function (template, row) {
            const filled = String(template || "").replace(/\{\{(\s*[A-Za-z_][A-Za-z0-9_]*)\.#\./g,
                                                         function (whole, entity) { return "{{" + entity + "." + (row || 1) + "."; });
            return render(filled, lookup);
        },
    };
}

// The list a repeating row is for, or null.
function repeatEntity(template) {
    const m = /\{\{\s*([A-Za-z_][A-Za-z0-9_]*)\.#\./.exec(String(template));
    return m ? m[1] : null;
}

function inputNames(template) {
    const names = [];
    const re = /\{\{\s*input\.([A-Za-z_][A-Za-z0-9_]*)\s*\}\}/g;
    let m;
    while ((m = re.exec(String(template))) !== null)
        if (names.indexOf(m[1]) < 0) names.push(m[1]);
    return names;
}

function inputLabel(name) {
    const words = name.replace(/_/g, " ").trim();
    return words.charAt(0).toUpperCase() + words.slice(1);
}

// "a/{{ x }}/b" with each folder name made safe and ".." dropped.
function subPath(source, template) {
    return source.fill(template).split(/[\\/]+/).map(function (part) { return validFilename(part); })
        .filter(function (part) { return part && part !== "." && part !== ".."; }).join("/");
}

// ── Office documents ───────────────────────────────────────────────────────

// Every COM object reached through `object`, kept in `held` so the run can
// release them all. Excel keeps running after Quit while any are held.
function tracked(object, held) {
    if (!object || (typeof object !== "object" && typeof object !== "function") || !object.$handle) return object;
    held.push(object);
    return new Proxy(object, {
        get: function (target, name) {
            if (name === "$handle") return target.$handle;
            const value = target[name];
            if (name === Symbol.iterator)
                return function () {
                    const out = [];
                    for (const item of { [Symbol.iterator]: value }) out.push(tracked(item, held));
                    return out[Symbol.iterator]();
                };
            if (typeof value === "function" && !value.$handle)
                return function () { return tracked(value.apply(null, arguments), held); };
            return tracked(value, held);
        },
        set: function (target, name, value) { target[name] = value; return true; },
        apply: function (target, self, args) { return tracked(Reflect.apply(target, self, args), held); },
    });
}

function* withOffice(progId, work, needs) {
    if (!pn.com) {
        yield ui.message("Creating documents from templates needs " + (needs || "Microsoft Office") + " on Windows.",
                         { kind: "error", title: progId });
        return null;
    }
    const held = [];
    try {
        return yield* work(tracked(pn.com.createObject(progId), held));
    } catch (e) {
        yield ui.message(e.message, { kind: "error", title: progId });
        return null;
    } finally {
        for (const object of held.reverse()) pn.com.release(object);
    }
}

const TOKEN = /\{\{[^{}]*\}\}/g;

function unique(list) {
    return list.filter(function (v, i) { return list.indexOf(v) === i; });
}

// Word keeps line breaks inside a paragraph as vertical tabs.
function wordText(value) { return String(value).replace(/\r\n|\r|\n/g, "\v"); }

// The main text, headers, footers, text boxes and notes.
function wordStories(doc) {
    const out = [];
    for (const first of doc.StoryRanges) {
        let story = first;
        for (let guard = 0; story && guard < 100; ++guard) {
            out.push(story);
            story = story.NextStoryRange;
        }
    }
    return out;
}

// A table row with an entity.#.column placeholder becomes one row for each
// row of that list, or goes away when the list is empty.
function expandWordRows(doc, source) {
    for (const table of doc.Tables) {
        for (let r = table.Rows.Count; r >= 1; --r) {
            const row = table.Rows(r);
            const entity = repeatEntity(row.Range.Text);
            if (!entity) continue;
            const cells = [];
            const count = row.Cells.Count;
            for (let c = 1; c <= count; ++c) cells.push(String(row.Cells(c).Range.Text).replace(/[\r\x07]+$/, ""));
            const items = source.rows(entity);
            if (!items.length) { row.Delete(); continue; }
            const fillRow = function (target, n) {
                cells.forEach(function (template, i) {
                    if (template.indexOf("{{") >= 0) target.Cells(i + 1).Range.Text = wordText(source.fill(template, n));
                });
            };
            // Each new row goes straight below the template row, so the last
            // item is added first.
            for (let n = items.length; n >= 2; --n) {
                const added = r < table.Rows.Count ? table.Rows.Add(table.Rows(r + 1)) : table.Rows.Add();
                fillRow(added, n);
            }
            fillRow(row, 1);
        }
    }
}

function replaceWordTokens(story, source) {
    for (const token of unique(String(story.Text || "").match(TOKEN) || [])) {
        const value = wordText(source.fill(token));
        const range = story.Duplicate;
        for (let guard = 0; guard < 1000; ++guard) {
            // Execute(FindText, MatchCase, MatchWholeWord, MatchWildcards,
            // MatchSoundsLike, MatchAllWordForms, Forward, Wrap=wdFindStop)
            if (!range.Find.Execute(token, true, false, false, false, false, true, 0)) break;
            range.Text = value;
            range.Collapse(0);   // wdCollapseEnd
        }
    }
}

function* wordDocument(word, template, kind, prepare) {
    word.Visible = false;
    word.DisplayAlerts = 0;
    const doc = word.Documents.Add(windowsPath(template));
    try {
        const plan = yield* prepare(wordStories(doc).map(function (s) { return String(s.Text || ""); }).join("\n"));
        if (!plan) return null;
        expandWordRows(doc, plan.source);
        for (const story of wordStories(doc)) replaceWordTokens(story, plan.source);
        doc.SaveAs2(windowsPath(plan.target), kind.format);
        return plan;
    } finally {
        doc.Close(0);   // wdDoNotSaveChanges
        if (word.Documents.Count === 0) word.Quit(0);
    }
}

// Every cell of the sheet whose text holds a placeholder.
function excelTokenCells(sheet) {
    const out = [];
    const used = sheet.UsedRange;
    // Find(What, After, LookIn=xlFormulas, LookAt=xlPart, SearchOrder=xlByRows, SearchDirection=xlNext, MatchCase)
    let cell = used.Find("{{", null, -4123, 2, 1, 1, false);
    if (!cell) return out;
    const first = cell.Address;
    for (let guard = 0; cell && guard < 100000; ++guard) {
        const formula = cell.Formula;
        if (typeof formula === "string" && formula.charAt(0) !== "=" && formula.indexOf("{{") >= 0)
            out.push({ row: cell.Row, column: cell.Column, text: formula });
        cell = used.FindNext(cell);
        if (!cell || cell.Address === first) break;
    }
    return out;
}

function setExcelCell(cell, value) {
    cell.Value = String(value).replace(/\r\n|\r/g, "\n").slice(0, 32000);
}

function fillSheet(sheet, cells, source) {
    const repeats = {};
    for (const c of cells) {
        const entity = repeatEntity(c.text);
        if (entity) (repeats[c.row] = repeats[c.row] || { entity: entity, cells: [] }).cells.push(c);
        else setExcelCell(sheet.Cells(c.row, c.column), source.fill(c.text));
    }
    // Bottom up, so inserted rows don't move the rows still to do.
    const rowNumbers = Object.keys(repeats).map(Number).sort(function (a, b) { return b - a; });
    for (const r of rowNumbers) {
        const group = repeats[r];
        const items = source.rows(group.entity);
        if (!items.length) { sheet.Rows(r).Delete(); continue; }
        if (items.length > 1) {
            const below = (r + 1) + ":" + (r + items.length - 1);
            sheet.Rows(below).Insert();
            sheet.Rows(r).Copy(sheet.Rows(below));
        }
        items.forEach(function (item, i) {
            for (const c of group.cells) setExcelCell(sheet.Cells(r + i, c.column), source.fill(c.text, i + 1));
        });
    }
}

function* excelDocument(excel, template, kind, prepare) {
    excel.Visible = false;
    excel.ScreenUpdating = false;
    excel.EnableEvents = false;
    excel.DisplayAlerts = false;
    const workbook = excel.Workbooks.Add(windowsPath(template));
    try {
        const sheets = [];
        for (const sheet of workbook.Worksheets) sheets.push({ sheet: sheet, cells: excelTokenCells(sheet) });
        const plan = yield* prepare(sheets.map(function (s) {
            return s.cells.map(function (c) { return c.text; }).join("\n");
        }).join("\n"));
        if (!plan) return null;
        for (const s of sheets) fillSheet(s.sheet, s.cells, plan.source);
        workbook.Worksheets(1).Activate();
        workbook.SaveAs(windowsPath(plan.target), kind.format);
        return plan;
    } finally {
        workbook.Close(false);
        if (excel.Workbooks.Count === 0) excel.Quit();
    }
}

// A table row with an entity.#.column placeholder becomes one row for each
// row of that list, or goes away when the list is empty.
function expandPptRows(presentation, source) {
    for (const slide of presentation.Slides) {
        for (const shape of slide.Shapes) {
            if (!shape.HasTable) continue;
            const table = shape.Table;
            const columns = table.Columns.Count;
            const cellRange = function (r, c) { return table.Cell(r, c).Shape.TextFrame.TextRange; };
            for (let r = table.Rows.Count; r >= 1; --r) {
                const cells = [];
                for (let c = 1; c <= columns; ++c) cells.push(String(cellRange(r, c).Text || ""));
                const entity = repeatEntity(cells.join("\n"));
                if (!entity) continue;
                const items = source.rows(entity);
                if (!items.length) { table.Rows(r).Delete(); continue; }
                const fillRow = function (target, n) {
                    cells.forEach(function (template, i) {
                        if (template.indexOf("{{") >= 0) cellRange(target, i + 1).Text = source.fill(template, n);
                    });
                };
                // Rows.Add(n) inserts above row n, so each new row goes straight
                // below the template row and the last item is added first.
                for (let n = items.length; n >= 2; --n) {
                    if (r < table.Rows.Count) table.Rows.Add(r + 1);
                    else table.Rows.Add();
                    fillRow(r + 1, n);
                }
                fillRow(r, 1);
            }
        }
    }
}

// The text of every shape and table cell on the slides.
function pptTextRanges(presentation) {
    const out = [];
    for (const slide of presentation.Slides) {
        for (const shape of slide.Shapes) {
            if (shape.HasTextFrame && shape.TextFrame.HasText) out.push(shape.TextFrame.TextRange);
            if (shape.HasTable) {
                const table = shape.Table;
                const rows = table.Rows.Count, columns = table.Columns.Count;
                for (let r = 1; r <= rows; ++r)
                    for (let c = 1; c <= columns; ++c) out.push(table.Cell(r, c).Shape.TextFrame.TextRange);
            }
        }
    }
    return out;
}

function* powerPointDocument(powerpoint, template, kind, prepare) {
    // Open(FileName, ReadOnly, Untitled, WithWindow): an untitled copy, no window.
    const presentation = powerpoint.Presentations.Open(windowsPath(template), -1, -1, 0);
    try {
        const ranges = pptTextRanges(presentation);
        const plan = yield* prepare(ranges.map(function (r) { return String(r.Text || ""); }).join("\n"));
        if (!plan) return null;
        expandPptRows(presentation, plan.source);
        for (const range of pptTextRanges(presentation)) {
            for (const token of unique(String(range.Text || "").match(TOKEN) || [])) {
                const value = plan.source.fill(token);
                for (let guard = 0; guard < 100 && range.Replace(token, value); ++guard) { /* next occurrence */ }
            }
        }
        presentation.SaveAs(windowsPath(plan.target), kind.format);
        return plan;
    } finally {
        presentation.Close();
        if (powerpoint.Presentations.Count === 0) powerpoint.Quit();
    }
}

// Today at 8:00 as an OLE date number, which Project reads as a date whatever
// the Windows date format.
function oleToday() {
    const now = new Date();
    return Date.UTC(now.getFullYear(), now.getMonth(), now.getDate()) / 86400000 + 25569 + 8 / 24;
}

// The schedule's title, subject, company, manager and comments, and every
// task's name and notes. A property this version of Project lacks is skipped.
function projectFields(schedule) {
    const out = [];
    const add = function (object, name) {
        try {
            const value = object[name];
            if (typeof value === "string") out.push({ object: object, name: name, text: value });
        } catch (e) { /* not in this version of Project */ }
    };
    for (const name of ["Title", "Subject", "Company", "Manager", "Comments"]) add(schedule, name);
    const tasks = [schedule.ProjectSummaryTask];
    for (const task of schedule.Tasks) tasks.push(task);
    for (const task of tasks) {
        if (!task) continue;   // a blank row
        add(task, "Name");
        add(task, "Notes");
    }
    return out;
}

// A task whose name or notes has an entity.#.column placeholder becomes one
// task for each row of that list, with the template task's outline level,
// duration and predecessors, or goes away when the list is empty.
function expandProjectTasks(schedule, source) {
    const repeats = [];
    for (const task of schedule.Tasks) {
        if (!task) continue;   // a blank row
        const name = String(task.Name || ""), notes = String(task.Notes || "");
        const entity = repeatEntity(name + "\n" + notes);
        if (entity) repeats.push({ task: task, name: name, notes: notes, entity: entity });
    }
    // The last first, so adding tasks below one never moves the ones above it.
    for (let k = repeats.length - 1; k >= 0; --k) {
        const r = repeats[k];
        const items = source.rows(r.entity);
        if (!items.length) { r.task.Delete(); continue; }
        const level = r.task.OutlineLevel;
        const duration = r.task.Duration;
        const predecessors = String(r.task.Predecessors || "");
        const fill = function (task, n) {
            task.Name = source.fill(r.name, n);
            if (r.notes) task.Notes = source.fill(r.notes, n).replace(/\r\n|\n/g, "\r");
        };
        // Tasks.Add(Name, Before) inserts at position Before, so each copy goes
        // straight below the template task and the last item is added first.
        const tasks = schedule.Tasks;
        for (let n = items.length; n >= 2; --n) {
            const id = r.task.ID;
            const added = id < tasks.Count ? tasks.Add(source.fill(r.name, n), id + 1) : tasks.Add(source.fill(r.name, n));
            if (added.OutlineLevel !== level) added.OutlineLevel = level;
            added.Duration = duration;
            if (predecessors) added.Predecessors = predecessors;
            fill(added, n);
        }
        fill(r.task, 1);
    }
}

function* projectDocument(msproject, template, kind, prepare) {
    msproject.Visible = false;
    msproject.DisplayAlerts = false;
    // FileOpenEx(Name, ReadOnly): read-only, so the template itself never changes.
    if (!msproject.FileOpenEx(windowsPath(template), true))
        throw new Error("Microsoft Project could not open " + template + ".");
    try {
        const schedule = msproject.ActiveProject;
        const plan = yield* prepare(projectFields(schedule).map(function (f) { return f.text; }).join("\n"));
        if (!plan) return null;
        expandProjectTasks(schedule, plan.source);
        // Read again: the repeated tasks are new, and an empty list's are gone.
        for (const f of projectFields(schedule)) {
            if (f.text.indexOf("{{") < 0) continue;
            f.object[f.name] = f.text.replace(TOKEN, function (token) { return plan.source.fill(token); })
                                     .replace(/\r\n|\n/g, "\r");
        }
        // Tasks without fixed dates move with the project's start.
        schedule.ProjectStart = oleToday();
        msproject.FileSaveAs(windowsPath(plan.target), kind.format);
        return plan;
    } finally {
        msproject.FileCloseEx(0);   // pjDoNotSave
        if (msproject.Projects.Count === 0) msproject.Quit(0);
    }
}

// ── Creating a document ────────────────────────────────────────────────────

function* createDocument(context) {
    const row = templateRows()[+context.parameter];
    if (!row) return;
    const title = String(row.menu || "Template");
    const templateSetting = String(row.template || "").trim();
    const template = isAbsolute(templateSetting) ? templateSetting : files.packagePath(templateSetting);
    const kind = KINDS[suffix(template)];
    if (!kind) {
        yield ui.message("MS Office Templates fills in Word, Excel, PowerPoint and MS Project templates. " + template + " is none of those.",
                         { kind: "error", title: title });
        return;
    }
    if (!isAbsolute(templateSetting) && !(yield files.exists(template))) {
        yield ui.message("The template " + template + " is missing. Choose another in Settings > MS Office Templates.",
                         { kind: "error", title: title });
        return;
    }
    const office = OFFICE[kind.office];

    const loaded = yield loadRecord(context.entity, context.key);
    if (!loaded.record) return;
    const project = yield projectOf(context.entity, loaded.record, loaded.schema);
    const defaults = yield app.defaults();
    const now = new Date();
    const isoToday = now.getFullYear() + "-" + pad(now.getMonth() + 1) + "-" + pad(now.getDate());
    const extras = {
        "template.name": title,
        "current.date": usDate(isoToday),
        "current.isoDate": isoToday,
        "preferences.managingCompanyName": defaults.managingCompanyName || "",
        "preferences.managerName": defaults.projectManagerName || "",
        "preferences.managerId": defaults.projectManagerId || "",
        // The names the earlier fixed templates used.
        "project.number": project ? text(project.project_number) : "",
        "project.name": project ? text(project.project_name) : "",
        "company.name": project ? text(project.company_id) : "",
    };
    // Column types, so money and percentages read as such.
    const schemas = [loaded.schema];
    if (project && project !== loaded.record) schemas.push(yield data.schema("projects"));
    const types = {};
    for (const schema of schemas) {
        types[schema.name] = {};
        for (const c of schema.columns || []) types[schema.name][c.name] = c.type;
    }
    const source = makeSource(loaded.record, project, types, extras);

    // The project's folder plus the configured subfolder, or a folder the
    // user picks when there is no project folder.
    let base = null;
    let useSubfolder = true;
    if (project) {
        const projectFolder = yield app.projectFolder(project.$key);
        if (projectFolder && (yield files.exists(projectFolder))) base = projectFolder.replace(/[\\/]+$/, "");
    }
    if (!base) {
        base = yield ui.pickFolder({ title: "Select a folder for the " + title });
        if (!base) return;
        base = base.replace(/[\\/]+$/, "");
        useSubfolder = false;
    }

    let existing = null;
    const prepare = function* (content) {
        const names = inputNames([row.folder, row.filename, content].join("\n"));
        if (names.length) {
            const answers = yield ui.showForm({ fields: names.map(function (n) { return { key: n, label: inputLabel(n), type: "text" }; }) },
                                              {}, { title: title, okText: "Create" });
            if (!answers) return null;
            names.forEach(function (n) { extras["input." + n] = answers[n] === null || answers[n] === undefined ? "" : String(answers[n]); });
        }
        const sub = useSubfolder ? subPath(source, row.folder || "") : "";
        const folder = base + (sub ? "/" + sub : "");
        const ext = kind.ext || suffix(template);
        let name = validFilename(source.fill(String(row.filename || "").trim() || "{{ projects.project_number }} {{ template.name }}"));
        if (suffix(name) === ext) name = name.slice(0, -(ext.length + 1));
        name = name.trim() || baseName(template);
        const target = folder + "/" + name + "." + ext;
        if (yield files.exists(target)) { existing = target; return null; }
        yield files.mkdir(folder);
        return { source: source, target: target, name: name + "." + ext };
    };

    const plan = yield withOffice(office.progId, function* (application) {
        return yield* office.open(application, template, kind, prepare);
    }, office.needs);
    if (existing) {
        if (yield ui.confirm(baseName(existing) + "." + suffix(existing) + " already exists. Open it?",
                             { title: title, okText: "Open" }))
            yield ui.openUrl(fileUrl(existing));
        return;
    }
    if (!plan) return;
    if (project) {
        yield data.save("project_locations", {
            project_id: { id: project.$key },
            location_type: office.locationType,
            location_description: plan.name,
            full_path: windowsPath(plan.target),
        });
    }
    yield ui.openUrl(fileUrl(plan.target));
}

// ── Open MS Project and stranded processes ─────────────────────────────────

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
    "open-ms-project": openMsProject,
    "kill-excel": function* () { yield* killAutomation("EXCEL.EXE", "Excel"); },
    "kill-word": function* () { yield* killAutomation("WINWORD.EXE", "Word"); },
};

export function* onMenu(menuId, context) {
    const handler = handlers[menuId];
    if (handler) yield* handler(context);
}

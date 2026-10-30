// IFS Cloud: project import, tracker item sync, SSRS status and tracker reports,
// and IFS shortcut menus in My Shortcuts. Replaces threads/ifssync_thread.py,
// plugins/ifscloud_plugin_settings.py, plugins/ifs_ssrs_generate_plugin.py and
// plugins/includes/ifs_tools.py. Ships separately from Project Notes.
import { log, secrets, settings, shortcuts, timers, ui } from "projectnotes";
import { hasSettings, importProjects, syncActivityTasks, syncTrackerItems } from "./ifs.mjs";
import { ifsUrl, reset, trace } from "./sso.mjs";
import { generateStatusReport, generateTrackerReport } from "./ssrs.mjs";

const SYNC_MINUTES = 3;

// Shortcut menus that open IFS pages and SSRS reports for the record you
// right-click. They live in My Shortcuts: on start this plugin adds each one it
// has not added before, with {{ ifs.address }} and {{ ifs.reportServer }}
// written in. One the user deletes there stays deleted.
const IFS_PAGE = "{{ ifs.address }}/main/ifsapplications/web/page/";
const REPORT_VIEWER = "http://{{ ifs.reportServer }}/ReportServer/Pages/ReportViewer.aspx?";
const DEFAULT_SHORTCUTS = [
    { menu: "Open Time Entry", submenu: "IFS Cloud", entity: "contacts",
      url: IFS_PAGE + "TimeRegistrationManager/TimeHeaderPage;$filter=%28startswith%28CompanyPersonRef%2FInternalDisplayName,'{{ contacts.name }}'%29%29" },
    { menu: "Open Project", submenu: "IFS Cloud", entity: "projects",
      url: IFS_PAGE + "ProjectDefinition/Form;path=0.1731531774.1886945984.458817809;$filter=%28%20ProjectId%20eq%20'{{ projects.project_number }}'%20%29" },
    { menu: "Open Gantt", submenu: "IFS Cloud", entity: "projects",
      url: IFS_PAGE + "ProjectGantt/Form;$filter=%28%20ProjectId%20eq%20'{{ projects.project_number }}'%20%29" },
    { menu: "Invoicing Plan", submenu: "IFS Cloud", entity: "projects",
      url: IFS_PAGE + "InvoicingPlan/List;$filter=%28%20ProjectId%20eq%20'{{ projects.project_number }}'%20%29;usedefaultfilter=false" },
    { menu: "Open PC Distribution", submenu: "IFS Cloud", entity: "projects",
      url: IFS_PAGE + "CPcDistribution/Form;$filter=%28%20DistributionId%20eq%20'{{ projects.project_number }}'%20%29" },
    { menu: "Open PO Table", submenu: "IFS Cloud", entity: "projects",
      url: IFS_PAGE + "CCustomerPOs/List;$filter=%28%20ProjectId%20eq%20'{{ projects.project_number }}'%20%29" },
    { menu: "Open Standard Price", submenu: "IFS Cloud", entity: "projects",
      url: IFS_PAGE + "StandardSalesPrice/Form;$filter=%28%20ProjectSalesPriceId%20eq%20'{{ projects.project_number }}'%20%29" },
    { menu: "Customer Parts Tracking", submenu: "Reports", entity: "projects",
      url: REPORT_VIEWER + "%2FProd%2FLead+Engineers%2FCustomer+Parts+Tracking&ProjectID={{ projects.project_number }}&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { menu: "Invalid Report Codes", submenu: "Reports", entity: "projects",
      url: REPORT_VIEWER + "%2FProd%2FProjects%2FInvalid+Report+Code+Assignments&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { menu: "Parts Ordered", submenu: "Reports", entity: "projects",
      url: REPORT_VIEWER + "%2FProd%2FProjects%2FProject+Parts+Ordered&ProjectID={{ projects.project_number }}&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { menu: "Project Report", submenu: "Reports", entity: "projects",
      url: REPORT_VIEWER + "%2FProd%2FProjects%2FProject+Report&ProjectId={{ projects.project_number }}&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { menu: "Project Status", submenu: "Reports", entity: "projects",
      url: REPORT_VIEWER + "%2FProd%2FLead+Engineers%2FProject+Status+Report&ProjectId={{ projects.project_number }}&ReportPeriod=30&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { menu: "Missing Time Entry", submenu: "Reports", entity: "projects",
      url: REPORT_VIEWER + "%2fProd%2fProjects%2fMissing+Time+Entry&rs:Command=Render&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { menu: "Item Tracker", submenu: "Reports", entity: "projects",
      url: REPORT_VIEWER + "%2FProd%2FLead+Engineers%2FIssues+List&ProjectId={{ projects.project_number }}&rs:ClearSession=true&rc:Zoom=Page%20Width" },
];

let timer = null;
let stopped = false;

function shortcutKey(row) {
    return [row.entity, row.submenu, row.menu].map(function (s) { return String(s || "").trim().toLowerCase(); }).join("\n");
}

// Writes the IFS address and report server into a shortcut's URL. Returns
// null while a setting it needs is empty.
function resolveShortcut(row) {
    const values = { "ifs.address": ifsUrl(), "ifs.reportServer": String(settings.get("ReportServer", "") || "").trim() };
    let url = String(row.url || "");
    for (const name of Object.keys(values)) {
        const pattern = new RegExp("\\{\\{\\s*" + name.replace(".", "\\.") + "\\s*\\}\\}", "g");
        if (url.search(pattern) < 0) continue;
        if (!values[name]) return null;
        url = url.replace(pattern, function () { return values[name]; });
    }
    return { menu: row.menu, submenu: row.submenu || "", entity: row.entity || "", url: url };
}

// Adds to My Shortcuts the shortcuts this plugin has not added before.
// Before 2.2 they lived in this plugin's own "Shortcuts" setting; that list,
// which the user may have changed, is moved once in place of the defaults.
function offerShortcuts() {
    const own = settings.get("Shortcuts");
    let seeded = settings.get("SeededShortcuts", []) || [];
    const waiting = [];
    const rows = [];
    (own !== null ? own || [] : DEFAULT_SHORTCUTS).forEach(function (row) {
        if (!row || !row.menu || !row.url || seeded.indexOf(shortcutKey(row)) >= 0) return;
        const resolved = resolveShortcut(row);
        if (resolved) rows.push(resolved);
        else waiting.push(row);
    });
    if (rows.length) {
        const added = shortcuts.add(rows);
        if (added) log.info("Added " + added + " IFS shortcut(s) to My Shortcuts.");
        seeded = seeded.concat(rows.map(shortcutKey));
    }
    // The user's own list stands in for the defaults, so those never follow;
    // the user's rows still waiting for a setting are not added yet.
    if (own !== null) {
        const pending = waiting.map(shortcutKey);
        seeded = seeded.concat(DEFAULT_SHORTCUTS.map(shortcutKey).filter(function (key) { return pending.indexOf(key) < 0; }));
    }
    settings.set("SeededShortcuts", seeded.filter(function (key, i) { return seeded.indexOf(key) === i; }));
    // Moved shortcuts that still need a setting wait here until it is filled in.
    if (own !== null) {
        if (waiting.length) settings.set("Shortcuts", waiting);
        else settings.remove("Shortcuts");
    }
}

// The Python plugin filled in the IFS address and the report server on first
// load. A value the user cleared stays that way.
function seedDefaults() {
    if (settings.get("URL") === null) settings.set("URL", "https://ifs.cornerstonecontrols.com");
    if (settings.get("ReportServer") === null) settings.set("ReportServer", "indvifsbi05");
}

// The domain password belongs in the credential store, not in settings.
function* movePasswordToSecrets() {
    const password = settings.get("DomainPassword", "");
    if (!password) return;
    yield secrets.set("DomainPassword", String(password));
    settings.remove("DomainPassword");
    log.info("Moved the report server password to the credential store.");
}

function* runSync(all) {
    const imported = yield importProjects(all);
    if (imported === null) return null;
    let counts = null;
    if (syncTrackerItems() && !stopped) {
        try {
            counts = yield syncActivityTasks();
        } catch (e) {
            log.warn("Syncing IFS tracker items failed: " + e.message);
        }
    }
    return { imported: imported, counts: counts };
}

function* background() {
    if (stopped || !hasSettings()) return;
    trace("sync", "background sync starting");
    try {
        const result = yield runSync(false);
        trace("sync", "background sync finished: " + JSON.stringify(result));
    } catch (e) {
        log.warn("IFS sync failed: " + e.message);
    }
}

function schedule() {
    if (timer) timer.cancel();
    timer = hasSettings() ? timers.every(SYNC_MINUTES, background) : null;
}

function* importNow() {
    if (!hasSettings()) {
        yield ui.message("Enter your IFS Cloud address in Settings > IFS Cloud first.", { kind: "warning", title: "IFS Not Configured" });
        return;
    }
    const result = yield ui.progress("Importing IFS projects", function* () { return yield runSync(true); }, { cancellable: false });
    if (!result) {
        yield ui.message("Could not reach or sign in to IFS Cloud. See the log for details.", { kind: "warning", title: "Import IFS Projects" });
        return;
    }
    let text = "Imported " + result.imported + " project(s).";
    if (result.counts)
        text += " Tracker items: " + result.counts.created + " new, " + result.counts.updated + " updated.";
    ui.notify(text, { kind: "success" });
}

export function* onStart() {
    trace("main", "starting; URL " + ifsUrl() + ", report server " + String(settings.get("ReportServer", "") || "(none)")
          + ", domain user " + String(settings.get("DomainUser", "") || "(none)"));
    stopped = false;
    seedDefaults();
    yield movePasswordToSecrets();
    offerShortcuts();
    schedule();
}

export function* onSettingsChanged(keys) {
    yield movePasswordToSecrets();
    if (!keys || keys.indexOf("URL") >= 0 || keys.indexOf("Realm") >= 0) reset();
    offerShortcuts();
    schedule();
}

export function onStop() {
    stopped = true;
    if (timer) timer.cancel();
}

export function* onMenu(menuId, context) {
    trace("menu", menuId + ": entity " + context.entity + ", key " + context.key
          + ", record " + (context.record ? "loaded" : "MISSING") + ", parameter " + (context.parameter || "(none)"));
    try {
        if (menuId === "import") yield importNow();
        else if (menuId === "status-report") yield generateStatusReport(context);
        else if (menuId === "tracker-report") yield generateTrackerReport(context);
        trace("menu", menuId + " finished");
    } catch (e) {
        trace("menu", menuId + " failed: " + e.message + (e.stack ? "\n" + e.stack : ""));
        throw e;
    }
}

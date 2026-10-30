// IFS Cloud: project import, tracker item sync, SSRS status and tracker reports,
// and IFS shortcut menus. Replaces threads/ifssync_thread.py,
// plugins/ifscloud_plugin_settings.py, plugins/ifs_ssrs_generate_plugin.py and
// plugins/includes/ifs_tools.py. Ships separately from Project Notes.
import { log, menus, secrets, settings, timers, ui } from "projectnotes";
import { replaceVariables } from "projectnotes/common";
import { hasSettings, importProjects, syncActivityTasks, syncTrackerItems } from "./ifs.mjs";
import { ifsUrl, reset, trace } from "./sso.mjs";
import { generateStatusReport, generateTrackerReport } from "./ssrs.mjs";

const SYNC_MINUTES = 3;

// Shortcut menus that open IFS pages and SSRS reports for the record you
// right-click. These were the default My Shortcuts entries; they start in the
// IFS Cloud settings, where you can change, add or remove them. A URL can use
// {{ ifs.address }} and {{ ifs.reportServer }} for those settings, so a new
// address carries over to every shortcut.
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

function shortcutValues() {
    return { "ifs.address": ifsUrl(), "ifs.reportServer": String(settings.get("ReportServer", "") || "").trim() };
}

function* openShortcut(context) {
    yield ui.openUrl(replaceVariables(context.parameter, context.record || {}, shortcutValues()));
}

// A shortcut that needs the IFS address or report server stays hidden until
// that setting has a value.
function registerShortcuts() {
    menus.clear();
    const values = shortcutValues();
    const items = [];
    const rows = settings.get("Shortcuts", []) || [];
    rows.forEach(function (row, i) {
        if (!row || !row.menu || !row.url) return;
        const missing = Object.keys(values).some(function (name) { return !values[name] && row.url.indexOf(name) >= 0; });
        if (missing) return;
        const entity = String(row.entity || "").trim();
        items.push({ id: "shortcut-" + i, title: row.menu, submenu: row.submenu || "",
                     context: entity ? "record" : "global", entity: entity || undefined,
                     parameter: row.url, handler: openShortcut });
    });
    if (items.length) menus.register(items);
}

// The Python plugin filled in the IFS address, the report server and the
// shortcuts on first load. A value the user cleared, or a shortcut list the
// user emptied, stays that way.
function seedDefaults() {
    if (settings.get("URL") === null) settings.set("URL", "https://ifs.cornerstonecontrols.com");
    if (settings.get("ReportServer") === null) settings.set("ReportServer", "indvifsbi05");
    if (settings.get("Shortcuts") === null) settings.set("Shortcuts", DEFAULT_SHORTCUTS);
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
    registerShortcuts();
    schedule();
}

export function* onSettingsChanged(keys) {
    yield movePasswordToSecrets();
    if (!keys || keys.indexOf("URL") >= 0 || keys.indexOf("Realm") >= 0) reset();
    registerShortcuts();
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

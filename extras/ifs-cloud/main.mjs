// IFS Cloud: project import, tracker item sync, SSRS status reports and IFS
// shortcut menus. Replaces threads/ifssync_thread.py,
// plugins/ifscloud_plugin_settings.py, plugins/ifs_ssrs_generate_plugin.py and
// plugins/includes/ifs_tools.py. Ships separately from Project Notes.
import { log, menus, secrets, settings, timers, ui } from "projectnotes";
import { replaceVariables } from "projectnotes/common";
import { hasSettings, importProjects, syncActivityTasks, syncTrackerItems } from "./ifs.mjs";
import { ifsUrl, reset } from "./sso.mjs";
import { generateStatusReport } from "./ssrs.mjs";

const SYNC_MINUTES = 3;

// Pages in IFS Cloud and reports on the report server, opened for the
// record you right-click. These were the default My Shortcuts entries.
const IFS_PAGES = [
    { title: "Open Time Entry", entity: "contacts",
      path: "/main/ifsapplications/web/page/TimeRegistrationManager/TimeHeaderPage;$filter=%28startswith%28CompanyPersonRef%2FInternalDisplayName,'[$contacts.name.1]'%29%29" },
    { title: "Open Project", entity: "projects",
      path: "/main/ifsapplications/web/page/ProjectDefinition/Form;path=0.1731531774.1886945984.458817809;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29" },
    { title: "Open Gantt", entity: "projects",
      path: "/main/ifsapplications/web/page/ProjectGantt/Form;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29" },
    { title: "Invoicing Plan", entity: "projects",
      path: "/main/ifsapplications/web/page/InvoicingPlan/List;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29;usedefaultfilter=false" },
    { title: "Open PC Distribution", entity: "projects",
      path: "/main/ifsapplications/web/page/CPcDistribution/Form;$filter=%28%20DistributionId%20eq%20'[$projects.project_number.1]'%20%29" },
    { title: "Open PO Table", entity: "projects",
      path: "/main/ifsapplications/web/page/CCustomerPOs/List;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29" },
    { title: "Open Standard Price", entity: "projects",
      path: "/main/ifsapplications/web/page/StandardSalesPrice/Form;$filter=%28%20ProjectSalesPriceId%20eq%20'[$projects.project_number.1]'%20%29" },
];
const REPORT_VIEWER = "/ReportServer/Pages/ReportViewer.aspx?";
const REPORTS = [
    { title: "Customer Parts Tracking", query: "%2FProd%2FLead+Engineers%2FCustomer+Parts+Tracking&ProjectID=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { title: "Invalid Report Codes", query: "%2FProd%2FProjects%2FInvalid+Report+Code+Assignments&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { title: "Parts Ordered", query: "%2FProd%2FProjects%2FProject+Parts+Ordered&ProjectID=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { title: "Project Report", query: "%2FProd%2FProjects%2FProject+Report&ProjectId=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { title: "Project Status", query: "%2FProd%2FLead+Engineers%2FProject+Status+Report&ProjectId=[$projects.project_number.1]&ReportPeriod=30&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { title: "Missing Time Entry", query: "%2fProd%2fProjects%2fMissing+Time+Entry&rs:Command=Render&rs:ClearSession=true&rc:Zoom=Page%20Width" },
    { title: "Item Tracker", query: "%2FProd%2FLead+Engineers%2FIssues+List&ProjectId=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width" },
];

let timer = null;
let stopped = false;

function* openShortcut(context) {
    yield ui.openUrl(replaceVariables(context.parameter, context.record || {}));
}

function registerShortcuts() {
    menus.clear();
    const items = [];
    const site = ifsUrl();
    if (site)
        IFS_PAGES.forEach(function (page, i) {
            items.push({ id: "ifs-page-" + i, title: page.title, context: "record", entity: page.entity, submenu: "IFS Cloud",
                         parameter: site + page.path, handler: openShortcut });
        });
    const server = String(settings.get("ReportServer", "") || "").trim();
    if (server)
        REPORTS.forEach(function (report, i) {
            items.push({ id: "report-" + i, title: report.title, context: "record", entity: "projects", submenu: "Reports",
                         parameter: "http://" + server + REPORT_VIEWER + report.query, handler: openShortcut });
        });
    if (items.length) menus.register(items);
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
    try {
        yield runSync(false);
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
    stopped = false;
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
    if (menuId === "import") yield importNow();
    else if (menuId === "status-report") yield generateStatusReport(context);
}

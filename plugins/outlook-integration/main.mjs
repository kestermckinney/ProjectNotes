// Outlook Integration: tracker items to Microsoft To Do, and project email
// archiving through Microsoft Graph or Outlook (classic). Replaces
// threads/outlooksync_thread.py, the Outlook parts of base_plugin.py, and the
// Outlook Integration settings dialog.
import * as pn from "projectnotes";
import { archiveFromGraph, archiveFromOutlook } from "./archive.mjs";
import { syncTrackerToTasks } from "./tasks.mjs";

const { app, log, menus, office365, settings, timers, ui } = pn;

function truthy(key) { return String(settings.get(key, "") || "").toLowerCase() === "true" || settings.get(key) === true; }
function useGraph() { return settings.get("IntegrationType", "") === "Office 365 Application"; }
function useOutlook() { return settings.get("IntegrationType", "") === "Outlook Application" && app.capabilities.has("com"); }

export function config() {
    return {
        syncWithDue: truthy("SyncToDoWithDue"),
        syncWithoutDue: truthy("SyncToDoDoWithoutDue"),
        backupEmails: truthy("BackupEmails"),
        inboxFolder: settings.get("BackupInBoxFolder", "") || "",
        sentFolder: settings.get("BackupSentFolder", "") || "",
    };
}

let stopped = false;
let timer = null;

function register() {
    menus.clear();
    const items = [];
    if (useGraph()) {
        items.push({ id: "sync-tasks", title: "Sync Tracker Items", context: "global", submenu: "Utilities", handler: syncAllNow });
        items.push({ id: "download-graph", title: "Download All Emails", context: "record", entity: "projects",
                     submenu: "Utilities", handler: downloadProjectGraph });
    }
    if (useOutlook()) {
        items.push({ id: "download-outlook", title: "Download Emails from Outlook", context: "record", entity: "projects",
                     submenu: "Utilities", handler: downloadProjectOutlook });
    }
    if (items.length) menus.register(items);

    if (timer) timer.cancel();
    timer = useGraph() ? timers.every(5, background) : null;
}

function* ensureSignedIn() {
    const status = yield office365.status();
    if (status.signedIn) return true;
    log.warn("Office 365 is not signed in; skipping the Outlook Integration sync.");
    return false;
}

// Every five minutes: a slice of tracker items and of project emails.
function* background() {
    if (stopped || !useGraph()) return;
    if (!(yield ensureSignedIn())) return;
    try {
        yield syncTrackerToTasks(config(), false);
        if (!stopped && config().backupEmails) yield archiveFromGraph(config(), null);
    } catch (e) {
        if (e.code === "consent-required") {
            log.warn(e.message);
            stopped = true;   // do not flood the log; reload the plugin after signing in
        } else {
            throw e;
        }
    }
}

function* syncAllNow() {
    if (!(yield ensureSignedIn())) {
        yield ui.message("Sign in to Office 365 in Settings > Office 365 Integration first.", { kind: "warning" });
        return;
    }
    const count = yield ui.progress("Syncing tracker items with Microsoft To Do", function* () {
        return yield syncTrackerToTasks(config(), true);
    }, { cancellable: false });
    ui.notify("Synced " + count + " tracker item(s).", { kind: "success" });
}

function* downloadProjectGraph(context) {
    if (!(yield ensureSignedIn())) {
        yield ui.message("Sign in to Office 365 in Settings > Office 365 Integration first.", { kind: "warning" });
        return;
    }
    const saved = yield ui.progress("Downloading emails for " + context.record.project_number, function* () {
        return yield archiveFromGraph(config(), context.record);
    }, { cancellable: false });
    ui.notify("Saved " + saved + " email(s).", { kind: "success" });
}

function* downloadProjectOutlook(context) {
    const ok = yield ui.confirm("This can take some time. Continue?", { title: "Download Emails from Outlook" });
    if (!ok) return;
    let folder = yield app.projectFolder(context.key);
    if (!folder) folder = yield ui.pickFolder({ title: "Select an output folder" });
    if (!folder) return;
    const saved = yield ui.progress("Archiving project emails", function* (progress) {
        return yield archiveFromOutlook(config(), context.record.project_number, folder, progress);
    }, { cancellable: false });
    ui.notify("Saved " + saved + " email(s).", { kind: "success" });
}

export function onStart() { stopped = false; register(); }
export function onSettingsChanged() { stopped = false; register(); }
export function onStop() { stopped = true; if (timer) timer.cancel(); }

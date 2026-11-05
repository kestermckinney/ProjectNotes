// Archive Mail: project email archiving through Microsoft Graph or Outlook
// (classic). Replaces threads/outlooksync_thread.py, the Outlook parts of
// base_plugin.py, and the Outlook Integration settings dialog.
import * as pn from "projectnotes";
import { archiveFromGraph, archiveFromOutlook, destination, projectDestination } from "./archive.mjs";

const { app, log, menus, office365, settings, timers, ui } = pn;

function useGraph() { return settings.get("IntegrationType", "") === "Office 365 Application"; }
function useOutlook() { return settings.get("IntegrationType", "") === "Outlook Application" && app.capabilities.has("com"); }

export function config() {
    return {
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
    log.warn("Office 365 is not signed in; skipping the Archive Mail sync.");
    return false;
}

// Every five minutes: a slice of project emails.
function* background() {
    if (stopped || !useGraph()) return;
    if (!(yield ensureSignedIn())) return;
    try {
        yield archiveFromGraph(config(), null);
    } catch (e) {
        if (e.code === "consent-required") {
            log.warn(e.message);
            stopped = true;   // do not flood the log; reload the plugin after signing in
        } else {
            throw e;
        }
    }
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
    // The project folder, local or Microsoft Teams, or a folder the user picks.
    let target = yield projectDestination(context.key);
    if (!target) {
        const folder = yield ui.pickFolder({ title: "Select an output folder" });
        if (!folder) return;
        target = destination(null, folder);
    }
    const saved = yield ui.progress("Archiving project emails", function* (progress) {
        return yield archiveFromOutlook(config(), context.record.project_number, target, progress);
    }, { cancellable: false });
    ui.notify("Saved " + saved + " email(s).", { kind: "success" });
}

export function onStart() { stopped = false; register(); }
export function onSettingsChanged() { stopped = false; register(); }
export function onStop() { stopped = true; if (timer) timer.cancel(); }

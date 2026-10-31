// My Shortcuts: configurable URL menus. Replaces plugins/myshortcuts_plugin.py
// and the My Shortcuts settings dialog.
import { app, menus, settings, ui, log } from "projectnotes";
import { replaceVariables, upgradeVariables } from "projectnotes/common";

// The Python plugin stored rows as {Menu, Submenu, URL, "Data Type"} with the
// stored table names. Translate them once to the published vocabulary.
const entityNames = { clients: "companies", people: "contacts", project_people: "project_team" };
const columnNames = { client_id: "company_id", client_name: "company_name", people_id: "contact_id",
                      person_id: "contact_id" };

function translateUrl(url) {
    return String(url || "").replace(/\[\$([A-Za-z0-9_]+)\.([A-Za-z0-9_]+)\.(\d+)\]/g, function (whole, table, column, row) {
        return "[$" + (entityNames[table] || table) + "." + (columnNames[column] || column) + "." + row + "]";
    });
}

// Earlier versions wrote [$managing_company_name] and so on; these are the
// names the {{ }} tokens use, shared with the email templates.
const extraNames = { managing_company_name: "preferences.managingCompanyName",
                     managing_manager_name: "preferences.managerName",
                     project_manager_id: "preferences.managerId" };

function toTokens(url) {
    return upgradeVariables(url).replace(/\{\{\s*(managing_company_name|managing_manager_name|project_manager_id)\s*\}\}/g,
                                        function (whole, name) { return "{{ " + extraNames[name] + " }}"; });
}

// Saved URLs written with [$...] placeholders become {{ }} tokens once.
function upgradeSaved() {
    const rows = settings.get("shortcuts", []) || [];
    let changed = false;
    rows.forEach(function (row) {
        if (row && typeof row.url === "string" && row.url.indexOf("[$") >= 0) {
            row.url = toTokens(row.url);
            changed = true;
        }
    });
    if (changed) settings.set("shortcuts", rows);
}

// Another plugin may have added its rows (marked addedBy) before this first
// start, so those alone don't mean the old shortcuts were already copied.
function migrate() {
    const current = settings.get("shortcuts");
    if (current !== null && (current.length === 0 || current.some(function (r) { return !r || !r.addedBy; }))) return;
    const legacy = settings.get("MyShortcuts");
    if (!legacy) return;
    try {
        const rows = JSON.parse(legacy).map(function (r) {
            const table = String(r["Data Type"] || "").trim();
            return { menu: r.Menu || "", submenu: r.Submenu || "", entity: entityNames[table] || table,
                     url: translateUrl(r.URL) };
        });
        const identity = function (r) { return [r.entity, r.submenu, r.menu].map(function (s) { return String(s || "").trim().toLowerCase(); }).join("\n"); };
        const seen = rows.map(identity);
        (current || []).forEach(function (r) { if (seen.indexOf(identity(r)) < 0) rows.push(r); });
        settings.set("shortcuts", rows);
        log.info("Migrated " + rows.length + " shortcut(s).");
    } catch (e) {
        log.error("Could not read the saved shortcuts: " + e.message);
    }
}

function register() {
    menus.clear();
    const rows = settings.get("shortcuts", []) || [];
    const items = [];
    rows.forEach(function (row, i) {
        if (!row || !row.menu || !row.url) return;
        const entity = String(row.entity || "").trim();
        items.push({ id: "shortcut-" + i, title: row.menu, submenu: row.submenu || "",
                     context: entity ? "record" : "global", entity: entity || undefined,
                     parameter: row.url, handler: open });
    });
    if (items.length) menus.register(items);
}

function* open(context) {
    const defaults = yield app.defaults();
    const url = replaceVariables(toTokens(context.parameter), context.record || {}, {
        "preferences.managingCompanyName": defaults.managingCompanyName,
        "preferences.managerName": defaults.projectManagerName,
        "preferences.managerId": defaults.projectManagerId,
    });
    const opened = yield ui.openUrl(url);
    if (!opened) yield ui.message("Could not open " + url, { kind: "warning" });
}

export function onStart() {
    migrate();
    upgradeSaved();
    register();
}

export function onSettingsChanged() {
    register();
}

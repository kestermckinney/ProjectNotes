// My Shortcuts: configurable URL menus. Replaces plugins/myshortcuts_plugin.py
// and the My Shortcuts settings dialog.
import { app, menus, settings, ui, log } from "projectnotes";
import { replaceVariables } from "projectnotes/common";

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

function migrate() {
    if (settings.get("shortcuts") !== null) return;
    const legacy = settings.get("MyShortcuts");
    if (!legacy) return;
    try {
        const rows = JSON.parse(legacy).map(function (r) {
            const table = String(r["Data Type"] || "").trim();
            return { menu: r.Menu || "", submenu: r.Submenu || "", entity: entityNames[table] || table,
                     url: translateUrl(r.URL) };
        });
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
    const url = replaceVariables(context.parameter, context.record || {}, {
        managing_company_name: defaults.managingCompanyName,
        managing_manager_name: defaults.projectManagerName,
        project_manager_id: defaults.projectManagerId,
    });
    const opened = yield ui.openUrl(url);
    if (!opened) yield ui.message("Could not open " + url, { kind: "warning" });
}

export function onStart() {
    migrate();
    register();
}

export function onSettingsChanged() {
    register();
}

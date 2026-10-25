// Probe plugin for tst_pluginhost: runs one scenario per menu id and reports
// {menuId, ok, result | error} through ui.message.
import { data, menus, settings, storage, files, http, auth, plugin, app, secrets, ui, PluginError } from "projectnotes";
import { render, replaceVariables, formatDate, parseDate, escapeHtml } from "projectnotes/common";

function report(menuId, ok, value) {
    return ui.message(JSON.stringify(ok ? { menuId, ok, result: value }
                                        : { menuId, ok, error: { code: value.code, message: value.message, permission: value.permission || null } }));
}

const scenarios = {
    *companies() {
        return yield data.list("companies", { orderBy: ["company_name"] });
    },
    *project(ctx) {
        return ctx.record;
    },
    *projectQuery() {
        return yield data.get("projects", { project_number: "P4000" },
            { include: [{ entity: "project_team", where: { role: "Lead" } }, "item_tracker"] });
    },
    *filtered() {
        const open = yield data.list("item_tracker", { where: { status: ["New", "Assigned"], priority: { $ne: "Low" } },
                                                       orderBy: [{ column: "item_number", descending: true }] });
        const dated = yield data.count("item_tracker", { date_due: { $gte: "2026-09-15" } });
        const byLookup = yield data.count("contacts", { company_id: { lookup: "Acme Corp" } });
        return { open: open.map(r => r.item_number), dated, byLookup };
    },
    *saveContact() {
        const saved = yield data.save("contacts", { name: "Zoe Quick", email: "zoe@example.com", company_id: { lookup: "Globex, Inc." } });
        const again = yield data.save("contacts", { name: "Zoe Quick", email: "zoe2@example.com" });
        return { first: saved.operation, second: again.operation, sameKey: saved.record.$key === again.record.$key,
                 company: saved.record.company_id, email: again.record.email };
    },
    *projectFolder() {
        const before = yield data.get("projects", { project_number: "P4000" });
        yield data.save("projects", { project_number: "P4000", folder_id: { lookup: "Favorites" } });
        const inFolder = yield data.count("projects", { folder_id: { lookup: "Favorites" } });
        const moved = yield data.get("projects", { project_number: "P4000" });
        yield data.save("projects", { project_number: "P4000", folder_id: null });
        const cleared = yield data.get("projects", { project_number: "P4000" });
        let unknown = null;
        try { yield data.save("projects", { project_number: "P4000", folder_id: { lookup: "No Such Folder" } }); }
        catch (e) { unknown = e.code; }
        return { before: before.folder_id, inFolder, moved: moved.folder_id, cleared: cleared.folder_id, unknown };
    },
    *readOnly() { yield data.save("contacts", { name: "X", company_name: "nope" }); },
    *storedName() { yield data.list("people"); },
    *badLookup() { yield data.save("contacts", { name: "Nobody", company_id: { lookup: "No Such Co" } }); },
    *notPermitted() { yield data.list("status_report_items"); },
    *badChoice() { yield data.save("item_tracker", { item_number: "99", item_type: "Tracker", priority: "Urgent", status: "New",
                                                          last_update: "2026-09-01", project_id: { lookup: "P4000" } }); },
    *txRollback() {
        try {
            yield data.transaction(function* (tx) {
                yield tx.save("companies", { company_name: "Rolled Back Co" });
                throw new Error("abort");
            });
        } catch (e) {}
        return yield data.count("companies", { company_name: "Rolled Back Co" });
    },
    *txCommit() {
        const r = yield data.transaction(function* (tx) {
            const c = yield tx.save("companies", { company_name: "Committed Co" });
            const p = yield tx.save("contacts", { name: "Committed Person", company_id: { id: c.record.$key } });
            return p.record.company_id.lookup;
        });
        return { lookup: r, count: yield data.count("companies", { company_name: "Committed Co" }) };
    },
    *removeBlocked() {
        // Jim Smith is the project's primary contact, so the delete is refused.
        const jim = yield data.get("contacts", { name: "Jim Smith" });
        yield data.remove("contacts", jim.$key);
    },
    *removeCascade() {
        const r = yield data.remove("item_tracker", "item-1");
        return r.removed.map(x => x.entity).sort();
    },
    *schema() {
        const s = yield data.schema("project_team");
        return { key: s.key, columns: s.columns.map(c => c.name), contact: s.columns.find(c => c.name === "contact_id").type,
                 ref: s.columns.find(c => c.name === "contact_id").ref.entity };
    },
    *settingsRoundTrip() {
        settings.set("color", "blue");
        settings.set("nested", { a: [1, 2] });
        return { color: settings.get("color"), missing: settings.get("missing", "dflt"), keys: settings.keys().sort(),
                 nested: settings.get("nested") };
    },
    *storageRoundTrip() {
        yield storage.writeJson("state/last.json", { n: 3 });
        const back = yield storage.readJson("state/last.json");
        let escaped = null;
        try { yield storage.readText("../outside.txt"); } catch (e) { escaped = e.code; }
        return { back, escaped, list: yield storage.list("state") };
    },
    *filesDenied() { yield files.readText("/etc/hostname"); },
    *filesPackage() { return yield files.readText(files.packagePath("plugin.json")).then(t => JSON.parse(t).id); },
    *httpDenied() { yield http.fetch("https://not-listed.example/x"); },
    *browserSignIn() {
        const r = yield auth.browserSignIn({ url: "https://sso.example.test/auth?client_id=probe",
                                             redirectUri: "http://127.0.0.1:" + settings.get("signInPort") + "/callback",
                                             timeoutSeconds: 10, serviceName: "Probe <SSO>" });
        return r.url;
    },
    *browserSignInBadHost() {
        yield auth.browserSignIn({ url: "https://not-listed.example/auth", redirectUri: "http://127.0.0.1:1/callback" });
    },
    *browserSignInBadRedirect() {
        yield auth.browserSignIn({ url: "https://sso.example.test/auth", redirectUri: "http://example.com:8080/callback" });
    },
    *timer() {
        return yield new Promise(resolve => setTimeout(() => resolve("fired"), 30));
    },
    // Builds strings and objects across many yields, as the IFS import does.
    // Qt 6.11's incremental garbage collector freed them mid-generator once
    // the host called back into the engine (here, a timer) between steps.
    *heldAcrossYields() {
        let rows = "", other = "";
        for (let i = 0; i < 1500; ++i) {
            const junk = [];
            for (let j = 0; j < 200; ++j) junk.push({ x: "v" + j + "_" + i, y: [j, i] });
            yield new Promise(resolve => setTimeout(resolve, 0));
            rows += "<row><column name=\"n\">" + i + "</column><column>" + junk[i % 200].x + "</column></row>";
            other += "<row>" + i + "</row>";
        }
        return (rows + other).length;
    },
    *common() {
        const rec = { $entity: "projects", project_number: "P4000", company_id: { id: "c1", lookup: "Acme Corp" },
                      project_team: [{ $entity: "project_team", role: "Lead", email: "a@x.test" }, { $entity: "project_team", role: "", email: "b@x.test" }] };
        const values = { "project.name": "A<B", "current.date": "09/02/2026" };
        return { tokens: replaceVariables("x {{ projects.project_number }} {{projects.company_id}} {{ project_team.1.role }} {{ project_team.2.role }} {{ report.date }} {{ project_team.*.email }}|{{ project_team.*.role }}|{{ project_team.*.nope }}", rec,
                                          { "report.date": "09/02/2026" }),
                 grammar: [render("{{ project.name }}|{{ current.date }}", values),
                           render("{{ project.name }}", values, { html: true }),
                           render("\\{{ project.name }}", values),
                           render("{{ constructor.x }}{{ project.name() }}{{ nope }}", values),
                           render("a {{ project.name", values)],
                 vars: replaceVariables("x [$projects.project_number.1] [$projects.company_id.1] [$project_team.role.1] [$project_team.role.2]", rec),
                 date: formatDate("2026-09-02", "MM/dd/yyyy"), parsed: parseDate("9/2/26"), html: escapeHtml("<a&b>") };
    },
    *arrays() {
        const p = yield data.get("projects", "project-p4000", { include: ["project_team"] });
        return { isArray: Array.isArray(p.project_team), apply: Math.max.apply(null, [1, 2].concat(p.project_team.map(() => 0))),
                 vars: replaceVariables("[$project_team.role.1]", p) !== "" };
    },
    *info() {
        return { id: plugin.id, platform: app.platform, email: app.capabilities.has("email.compose"),
                 com: app.capabilities.has("com"), secret: yield secrets.get("token") };
    },
    *dynamicMenu() {
        menus.register({ id: "dyn", title: "Dynamic", context: "record", entity: "contacts", handler: ctx => report("dyn", true, ctx.record.name) });
        return menus.list().map(m => m.id);
    },
    *runaway() { while (true) {} },
    *xml() {
        const xml = yield data.exportXml("companies", "client-acme");
        return { hasClients: xml.indexOf('name="clients"') > 0, hasName: xml.indexOf("Acme Corp") > 0 };
    },
};

export function* onMenu(menuId, ctx) {
    try {
        const result = yield scenarios[menuId](ctx);
        yield report(menuId, true, result);
    } catch (e) {
        yield report(menuId, false, e);
    }
}

export function onStart() { console.log("probe started"); }

// Team Member Quick Add: create a contact (and its company, if new) and add
// it to the project team. Replaces plugins/team_member_quick_add_plugin.py.
import { data, ui } from "projectnotes";

export function* onMenu(menuId, context) {
    const projectKey = context.record && context.record.project_id ? context.record.project_id.id : null;
    if (!projectKey) {
        yield ui.message("Right-click a member of the project's team to add another.", { kind: "warning" });
        return;
    }
    const companies = yield data.list("companies", { fields: ["company_name"], orderBy: ["company_name"] });
    const result = yield ui.showDialog("QuickAddDialog.qml", {
        companies: companies.map(c => c.company_name)
    }, { title: "Team Member Quick Add", width: 460, height: 430 });
    if (!result) return;

    const name = String(result.name || "").trim();
    if (!name) return;
    const company = String(result.newCompany || result.company || "").trim();

    yield data.transaction(function* (tx) {
        if (company) yield tx.save("companies", { company_name: company });
        const contact = yield tx.save("contacts", {
            name: name,
            email: result.email || null,
            office_phone: result.officePhone || null,
            cell_phone: result.mobilePhone || null,
            role: result.role || null,
            company_id: company ? { lookup: company } : null
        });
        yield tx.save("project_team", {
            project_id: { id: projectKey },
            contact_id: { id: contact.record.$key },
            role: result.role || null
        });
    });
    ui.notify(name + " was added to the team.", { kind: "success" });
}

// UI test plugin for tst_uismoke: opens a schema form and a plugin QML dialog
// and records what each returned in its settings.
import { ui, settings } from "projectnotes";

export function* onMenu(menuId) {
    if (menuId === "form") {
        const values = yield ui.showForm({ fields: [
            { key: "name", label: "Name", type: "text", required: true },
            { key: "when", label: "When", type: "date" },
            { key: "kind", label: "Kind", type: "choice", choices: ["A", "B"] },
            { key: "done", label: "Done", type: "checkbox" },
            { key: "notes", label: "Notes", type: "multiline" },
            { key: "rows", label: "Rows", type: "table", columns: [{ key: "a", label: "A", type: "text" }] },
        ] }, { name: "first" }, { title: "Form Test" });
        settings.set("formResult", values);
    } else if (menuId === "dialog") {
        const result = yield ui.showDialog("Hello.qml", { greeting: "hi" }, {
            title: "Dialog Test",
            handlers: { shout: function (text) { return text.toUpperCase(); } },
        });
        settings.set("dialogResult", result);
    }
}

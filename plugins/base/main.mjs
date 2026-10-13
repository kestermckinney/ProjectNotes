// Base Plugins: clipboard helpers and the Script Editor launcher.
// Replaces plugins/base_plugin.py and the Editor settings dialog.
import * as pn from "projectnotes";

// process is only present when the build can launch programs, so it is read
// from the namespace rather than imported by name.
const { ui, menus, settings, app } = pn;

function* copy(record, column) {
    const value = record ? record[column] : null;
    if (!value) {
        ui.notify("This record has no " + (column === "email" ? "email address" : "path") + ".", { kind: "warning" });
        return;
    }
    ui.clipboard.setText(String(value));
    ui.notify("Copied " + value);
}

function registerEditorMenu() {
    // process.launch is not available in every build (for example the App Store build).
    if (!app.capabilities.has("process.launch")) return;
    menus.register({ id: "script-editor", title: "Script Editor", context: "global", submenu: "Utilities",
                     handler: openEditor });
}

function* openEditor() {
    const editor = settings.get("EditorPath");
    if (!editor) {
        yield ui.message("Choose an editor in Settings > Script Editor first.", { title: "Editor Not Specified", kind: "warning" });
        return;
    }
    yield pn.process.launch(editor, []);
}

export function onStart() { registerEditorMenu(); }

export function* onMenu(menuId, context) {
    if (menuId === "copy-path") yield copy(context.record, "full_path");
    else yield copy(context.record, "email");
}

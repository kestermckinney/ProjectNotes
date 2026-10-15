// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ProjectNotesDesktop

// Renders a plugin form schema ({fields: [...]}, see projectnotes.d.ts
// FormSchema) with the application's own controls, for ui.showForm(),
// ui.prompt()/choose() and schema-based plugin Settings sections.
//
// Values use the plugin API's types: dates are YYYY-MM-DD strings, checkboxes
// booleans, numbers numbers, tables arrays of row objects.
ColumnLayout {
    id: form
    property var fields: []
    property var values: ({})
    // Emitted when any value changes (Settings sections save immediately).
    signal changed(string key, var value)

    property var _values: ({})
    property int _revision: 0
    spacing: 10

    onValuesChanged: _reset()
    Component.onCompleted: _reset()

    function _reset() {
        var copy = {}
        var source = values || {}
        for (var k in source) copy[k] = source[k]
        _values = copy
        _revision++
    }

    function value(key) { return _values[key] }
    function setValue(key, v) {
        _values[key] = v
        _revision++
        changed(key, v)
    }
    function collect() {
        var out = {}
        for (var i = 0; i < fields.length; ++i) {
            var f = fields[i]
            if (f.type === "section" || f.type === "note") continue
            out[f.key] = _values[f.key] === undefined ? null : _values[f.key]
        }
        return out
    }
    // The label of the first visible required field with no value, or "".
    function missingRequired() {
        for (var i = 0; i < fields.length; ++i) {
            var f = fields[i]
            if (!f.required || !_visible(f)) continue
            var v = _values[f.key]
            if (v === undefined || v === null || v === "" || (Array.isArray(v) && v.length === 0))
                return f.label || f.key
        }
        return ""
    }
    function _visible(f) {
        if (!f.visibleWhen) return true
        for (var k in f.visibleWhen)
            if (_values[k] !== f.visibleWhen[k]) return false
        return true
    }
    function _isoToUs(iso) {
        var m = /^(\d{4})-(\d{2})-(\d{2})/.exec(iso || "")
        return m ? m[2] + "/" + m[3] + "/" + m[1] : ""
    }
    function _usToIso(us) {
        var m = /^(\d{1,2})\/(\d{1,2})\/(\d{4})$/.exec((us || "").trim())
        if (!m) return null
        return m[3] + "-" + ("0" + m[1]).slice(-2) + "-" + ("0" + m[2]).slice(-2)
    }

    Repeater {
        model: form.fields
        delegate: Loader {
            id: fieldLoader
            required property var modelData
            readonly property var field: modelData
            Layout.fillWidth: true
            visible: (form._revision, form._visible(field))
            active: visible
            sourceComponent: {
                switch (field.type) {
                case "multiline": return multilineField
                case "date": return dateField
                case "checkbox": return checkboxField
                case "choice": return choiceField
                case "combo": return comboField
                case "file":
                case "folder": return pathField
                case "contact": return contactField
                case "table": return tableField
                case "section": return sectionField
                case "note": return noteField
                default: return textField
                }
            }
        }
    }

    Component {
        id: textField
        ColumnLayout {
            readonly property var field: parent ? parent.field : ({})
            spacing: 2
            FormField {
                Layout.fillWidth: true
                label: field.label + (field.required ? " *" : "")
                placeholder: field.placeholder || ""
                text: { form._revision; var v = form.value(field.key); return v === undefined || v === null ? "" : String(v) }
                Component.onCompleted: {
                    if (field.type === "password") inputItem.echoMode = TextInput.Password
                    if (field.type === "number") inputItem.inputMethodHints = Qt.ImhFormattedNumbersOnly
                }
                onEdited: (t) => form.setValue(field.key, field.type === "number" ? (t === "" ? null : Number(t)) : t)
            }
            Label { visible: !!field.help; text: field.help || ""; color: Theme.text3; font.pixelSize: Theme.fontSm; wrapMode: Text.Wrap; Layout.fillWidth: true }
        }
    }

    Component {
        id: multilineField
        ColumnLayout {
            readonly property var field: parent ? parent.field : ({})
            spacing: 4
            Label { text: field.label + (field.required ? " *" : ""); color: Theme.text2; font.pixelSize: Theme.fontSm }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                TextArea {
                    wrapMode: TextEdit.Wrap
                    placeholderText: field.placeholder || ""
                    color: Theme.text
                    background: Rectangle { color: Theme.surface; border.color: Theme.border; radius: Theme.radiusSm }
                    text: { form._revision; var v = form.value(field.key); return v === undefined || v === null ? "" : String(v) }
                    onTextChanged: if (activeFocus) form.setValue(field.key, text)
                }
            }
            Label { visible: !!field.help; text: field.help || ""; color: Theme.text3; font.pixelSize: Theme.fontSm; wrapMode: Text.Wrap; Layout.fillWidth: true }
        }
    }

    Component {
        id: dateField
        DateField {
            readonly property var field: parent ? parent.field : ({})
            label: field.label + (field.required ? " *" : "")
            text: { form._revision; return form._isoToUs(form.value(field.key)) }
            onEdited: (v) => form.setValue(field.key, form._usToIso(v))
        }
    }

    Component {
        id: checkboxField
        ThemedCheckBox {
            readonly property var field: parent ? parent.field : ({})
            text: field.label
            checked: { form._revision; return !!form.value(field.key) }
            onToggled: form.setValue(field.key, checked)
        }
    }

    Component {
        id: choiceField
        ComboField {
            readonly property var field: parent ? parent.field : ({})
            label: field.label + (field.required ? " *" : "")
            options: field.choices || []
            includeNone: !field.required
            value: { form._revision; var v = form.value(field.key); return v === undefined || v === null ? "" : String(v) }
            onActivated: (v) => form.setValue(field.key, v === noneLabel ? null : v)
        }
    }

    Component {
        id: comboField
        ComboField {
            readonly property var field: parent ? parent.field : ({})
            label: field.label + (field.required ? " *" : "")
            options: field.choices || []
            searchable: true
            value: { form._revision; var v = form.value(field.key); return v === undefined || v === null ? "" : String(v) }
            onActivated: (v) => form.setValue(field.key, v)
        }
    }

    Component {
        id: pathField
        ColumnLayout {
            id: pathRow
            readonly property var field: parent ? parent.field : ({})
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                FormField {
                    id: pathText
                    Layout.fillWidth: true
                    label: field.label + (field.required ? " *" : "")
                    placeholder: field.placeholder || ""
                    text: { form._revision; var v = form.value(field.key); return v === undefined || v === null ? "" : String(v) }
                    onEdited: (t) => form.setValue(field.key, t)
                }
                Button {
                    Layout.alignment: Qt.AlignBottom
                    text: qsTr("Browse…")
                    onClicked: field.type === "folder" ? folderPicker.open() : filePicker.open()
                }
            }
            FileDialog {
                id: filePicker
                nameFilters: pathRow.field.filters && pathRow.field.filters.length ? pathRow.field.filters : [qsTr("All files (*)")]
                onAccepted: {
                    DesktopAppController.rememberLocation(selectedFile)
                    form.setValue(pathRow.field.key, PluginUiController.urlToPath(selectedFile))
                }
            }
            FolderDialog {
                id: folderPicker
                onAccepted: {
                    DesktopAppController.rememberLocation(selectedFolder)
                    form.setValue(pathRow.field.key, PluginUiController.urlToPath(selectedFolder))
                }
            }
        }
    }

    Component {
        id: contactField
        ColumnLayout {
            id: contactRow
            readonly property var field: parent ? parent.field : ({})
            spacing: 4
            Label { text: field.label + (field.required ? " *" : ""); color: Theme.text2; font.pixelSize: Theme.fontSm }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    color: Theme.text
                    text: {
                        form._revision
                        var key = form.value(contactRow.field.key)
                        return key ? DesktopAppController.contactNameForId(key) : qsTr("(none)")
                    }
                }
                Button { text: qsTr("Choose…"); onClicked: picker.open() }
            }
            ContactPickerDialog {
                id: picker
                parent: Overlay.overlay
                headingText: contactRow.field.label || qsTr("Choose Contact")
                reload: function () { return DesktopAppController.contactList() }
                onPicked: (contact) => form.setValue(contactRow.field.key, contact.id)
            }
        }
    }

    Component {
        id: tableField
        ColumnLayout {
            id: table
            readonly property var field: parent ? parent.field : ({})
            readonly property var columns: field.columns || []
            readonly property var rows: { form._revision; var v = form.value(field.key); return Array.isArray(v) ? v : [] }
            spacing: 4
            function setCell(row, key, value) {
                var copy = rows.map(function (r) { return Object.assign({}, r) })
                copy[row][key] = value
                form.setValue(field.key, copy)
            }
            Label { text: field.label + (field.required ? " *" : ""); color: Theme.text2; font.pixelSize: Theme.fontSm }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Repeater {
                    model: table.columns
                    Label {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: modelData.label
                        color: Theme.text3; font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
                    }
                }
                Item { Layout.preferredWidth: 28 }
            }
            Repeater {
                model: table.rows.length
                RowLayout {
                    id: tableRow
                    required property int index
                    Layout.fillWidth: true
                    spacing: 4
                    Repeater {
                        model: table.columns
                        TextField {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            color: Theme.text
                            background: Rectangle { color: Theme.surface; border.color: Theme.border; radius: Theme.radiusSm }
                            text: { var v = table.rows[tableRow.index][modelData.key]; return v === undefined || v === null ? "" : String(v) }
                            onEditingFinished: table.setCell(tableRow.index, modelData.key, text)
                        }
                    }
                    ToolButton {
                        Layout.preferredWidth: 28
                        text: "×"
                        onClicked: {
                            var copy = table.rows.slice()
                            copy.splice(tableRow.index, 1)
                            form.setValue(table.field.key, copy)
                        }
                    }
                }
            }
            Button {
                text: qsTr("Add Row")
                onClicked: {
                    var row = {}
                    for (var i = 0; i < table.columns.length; ++i) row[table.columns[i].key] = ""
                    form.setValue(table.field.key, table.rows.concat([row]))
                }
            }
        }
    }

    Component {
        id: sectionField
        Label {
            readonly property var field: parent ? parent.field : ({})
            topPadding: 6
            text: field.label
            color: Theme.text
            font.pixelSize: Theme.fontLg
            font.weight: Font.DemiBold
        }
    }

    Component {
        id: noteField
        Label {
            readonly property var field: parent ? parent.field : ({})
            text: field.label
            color: Theme.text2
            wrapMode: Text.Wrap
        }
    }
}

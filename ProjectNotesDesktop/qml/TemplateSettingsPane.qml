// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// QML is intentionally only a form over TemplateEditorModel.  Token parsing,
// validation, persistence, and profile/database scoping all stay in C++.
ColumnLayout {
    id: pane
    property var templateModel: null
    spacing: 10

    // The settings page uses the same surface, border, indicator, and popup
    // treatment as the application's standard dropdowns rather than the stock
    // Controls appearance.

    // Match FormField's application-wide text-input treatment while keeping
    // direct access to the editor for template-token insertion.
    component TemplateTextField: TextField {
        id: control
        property bool spellCheckEnabled: false
        property var insertFields: []
        signal insertFieldRequested(string path)
        implicitHeight: 30
        leftPadding: 9
        rightPadding: 9
        verticalAlignment: Text.AlignVCenter
        color: Theme.text
        placeholderTextColor: Theme.text3
        font.pixelSize: Theme.fontBody
        selectByMouse: true
        background: Rectangle {
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: control.activeFocus ? Theme.accent : Theme.border
        }
        SpellCheckField {
            target: control
            spellCheckEnabled: control.spellCheckEnabled
            insertFields: control.insertFields
            onInsertFieldRequested: (path) => control.insertFieldRequested(path)
        }
    }

    function insertField(editor, path, target) {
        if (!pane.templateModel || !editor)
            return
        const token = "{{ " + path + " }}"
        const start = Math.min(editor.selectionStart, editor.selectionEnd)
        const end = Math.max(editor.selectionStart, editor.selectionEnd)
        if (end > start)
            editor.remove(start, end)
        editor.insert(start, token)
        editor.cursorPosition = start + token.length
        if (target === "subject")
            pane.templateModel.draftSubject = editor.text
    }

    // Leaving a draft is a commit point.  Store it first so an incomplete
    // draft is never lost, then validate it before allowing the user to move
    // on.  The caller uses the return value to keep its current page/selection
    // in place when there is something to fix.
    function saveAndValidate() {
        if (!pane.templateModel)
            return true
        const saved = pane.templateModel.save()
        const valid = pane.templateModel.validateDraft()
        if (saved && valid)
            return true
        templateValidationDialog.message = pane.templateModel.diagnostic !== ""
                ? pane.templateModel.diagnostic
                : qsTr("The template could not be saved or validated.")
        templateValidationDialog.open()
        return false
    }

    Label {
        text: qsTr("Email Templates")
        color: Theme.text
        font.pixelSize: Theme.fontXl
        font.weight: Font.DemiBold
    }
    Label {
        Layout.fillWidth: true
        text: qsTr("Templates are stored for this database and this Project Notes profile. Fields are inserted literally and are resolved only when a workflow has its real context.")
        color: Theme.text2
        wrapMode: Text.WordWrap
    }

    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Workflow"); color: Theme.text2 }
        StyledComboBox {
            id: workflow
            Layout.preferredWidth: 220
            model: [
                { value: "send-meeting-notes", label: qsTr("Send Meeting Notes") },
                { value: "tracker-items-report", label: qsTr("Tracker Items Report") },
                { value: "status-report", label: qsTr("Status Report") },
                { value: "meeting-notes-report", label: qsTr("Meeting Notes Report") },
                { value: "project-email", label: qsTr("Project Email") },
                { value: "share-location", label: qsTr("Share Location") }
            ]
            textRole: "label"
            valueRole: "value"
            currentIndex: {
                if (!pane.templateModel) return 0
                for (let i = 0; i < model.length; ++i)
                    if (model[i].value === pane.templateModel.workflow) return i
                return 0
            }
            onActivated: {
                if (!pane.templateModel || currentValue === pane.templateModel.workflow)
                    return
                if (pane.saveAndValidate())
                    pane.templateModel.workflow = currentValue
            }
        }
        Item { Layout.fillWidth: true }
        Button { text: qsTr("Reset"); enabled: pane.templateModel && pane.templateModel.selectedTemplateId !== ""; onClicked: pane.templateModel.resetSelected() }
    }

    RowLayout {
        Layout.fillWidth: true
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Label {
                Layout.fillWidth: true
                text: qsTr("This is the one editable template for the selected built-in workflow.")
                color: Theme.text3
                font.pixelSize: Theme.fontSm
                wrapMode: Text.WordWrap
            }
            Label { text: qsTr("Template name"); color: Theme.text2 }
            TemplateTextField {
                Layout.fillWidth: true
                text: pane.templateModel ? pane.templateModel.draftName : ""
                enabled: pane.templateModel !== null
                onTextEdited: pane.templateModel.draftName = text
            }
            Label { text: qsTr("Subject"); color: Theme.text2 }
            TemplateTextField {
                id: subject
                Layout.fillWidth: true
                text: pane.templateModel ? pane.templateModel.draftSubject : ""
                enabled: pane.templateModel !== null
                spellCheckEnabled: true
                insertFields: pane.templateModel ? pane.templateModel.availableFields : []
                onTextEdited: pane.templateModel.draftSubject = text
                onInsertFieldRequested: (path) => pane.insertField(subject, path, "subject")
            }
        }
    }

    Label { text: qsTr("Rich body"); color: Theme.text2 }
    Label { text: pane.templateModel && pane.templateModel.workflow === "project-email"
                  ? qsTr("Configure the project email subject and body. Person fields refer to the person right-clicked; they are empty for a project-team email.")
                  : qsTr("Include exactly one {{ content.body }} protected content block. It preserves the shared content while you add prose around it."); color: Theme.text3; font.pixelSize: Theme.fontSm; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    RichTextEditor {
        id: richBody
        minimumHeight: 120
        placeholderText: qsTr("Rich HTML template prose")
        value: pane.templateModel ? pane.templateModel.draftRichBody : ""
        editingEnabled: pane.templateModel !== null
        insertFields: pane.templateModel ? pane.templateModel.availableBodyFields : []
        onValueEdited: if (pane.templateModel) pane.templateModel.draftRichBody = value
        onInsertFieldRequested: (path) => pane.insertField(richBody.editor, path, "richBody")
    }
    Label {
        Layout.fillWidth: true
        visible: pane.templateModel && pane.templateModel.diagnostic !== ""
        text: pane.templateModel ? pane.templateModel.diagnostic : ""
        color: Theme.red
        wrapMode: Text.WordWrap
    }
    // A modal dialog, rather than only the inline diagnostic, so a validation
    // message that blocks navigation is not easy to miss.
    ThemedMessageDialog {
        id: templateValidationDialog
        title: qsTr("Email template needs attention")
    }
}

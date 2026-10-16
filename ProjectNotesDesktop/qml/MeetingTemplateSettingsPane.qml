// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// The meeting counterpart of TemplateSettingsPane.  Where email templates
// pick one of the fixed workflows, this pane picks one of the user's own
// meeting types and can add, duplicate and delete them.  As with email
// templates, QML is only a form: validation and persistence stay in
// MeetingTemplateEditorModel.
ColumnLayout {
    id: pane
    property var templateModel: null
    readonly property bool _editing: pane.templateModel !== null && pane.templateModel.hasDraft
    spacing: 10

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

    // Same commit-point contract as TemplateSettingsPane.saveAndValidate():
    // store the draft first so nothing is lost, then validate it before the
    // caller lets the user move to another meeting type or settings tab.
    function saveAndValidate() {
        if (!pane.templateModel)
            return true
        const saved = pane.templateModel.save()
        const valid = pane.templateModel.validateDraft()
        if (saved && valid)
            return true
        validationDialog.message = pane.templateModel.diagnostic !== ""
                ? pane.templateModel.diagnostic
                : qsTr("The meeting type could not be saved or validated.")
        validationDialog.open()
        return false
    }

    function _indexOf(options, value) {
        for (let i = 0; i < options.length; ++i)
            if (options[i].value === value) return i
        return -1
    }

    Label {
        text: qsTr("Meeting Templates")
        color: Theme.text
        font.pixelSize: Theme.fontXl
        font.weight: Font.DemiBold
    }
    Label {
        Layout.fillWidth: true
        text: qsTr("Each meeting type sets the invitation subject, agenda and duration used when you schedule a meeting from a project. Meeting types are stored in this database. Fields are inserted literally and are resolved from the project when the meeting is scheduled.")
        color: Theme.text2
        wrapMode: Text.WordWrap
    }

    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Meeting type"); color: Theme.text2 }
        StyledComboBox {
            id: meetingType
            objectName: "meetingTypeCombo"
            Layout.preferredWidth: 260
            enabled: pane.templateModel !== null && count > 0
            model: pane.templateModel ? pane.templateModel.templates : []
            textRole: "name"
            valueRole: "id"
            displayText: count > 0 ? currentText : qsTr("No meeting types")
            currentIndex: {
                if (!pane.templateModel) return -1
                const rows = pane.templateModel.templates
                for (let i = 0; i < rows.length; ++i)
                    if (rows[i].id === pane.templateModel.selectedTemplateId) return i
                return -1
            }
            onActivated: {
                if (!pane.templateModel || currentValue === pane.templateModel.selectedTemplateId)
                    return
                const target = currentValue
                if (pane.saveAndValidate())
                    pane.templateModel.selectTemplate(target)
            }
        }
        Item { Layout.fillWidth: true }
        Button {
            objectName: "newMeetingTypeButton"
            text: qsTr("New")
            enabled: pane.templateModel !== null
            onClicked: {
                if (pane.saveAndValidate()) {
                    pane.templateModel.beginNew()
                    nameField.forceActiveFocus()
                }
            }
        }
        Button {
            text: qsTr("Duplicate")
            enabled: pane._editing
            onClicked: {
                if (pane.saveAndValidate())
                    pane.templateModel.duplicateSelected()
            }
        }
        Button {
            text: qsTr("Delete")
            enabled: pane._editing
            onClicked: {
                deleteDialog.message = qsTr("Delete “%1”? This cannot be undone.")
                        .arg(pane.templateModel.draftName !== "" ? pane.templateModel.draftName : qsTr("New meeting type"))
                deleteDialog.open()
            }
        }
        Button {
            objectName: "resetMeetingTemplatesButton"
            text: qsTr("Reset to Defaults")
            enabled: pane.templateModel !== null
            onClicked: resetDialog.open()
        }
    }

    Label {
        Layout.fillWidth: true
        visible: pane.templateModel !== null && !pane.templateModel.hasDraft
        text: qsTr("There are no meeting types yet. Choose New to add one.")
        color: Theme.text3
        wrapMode: Text.WordWrap
    }

    GridLayout {
        Layout.fillWidth: true
        visible: pane._editing
        columns: 2
        columnSpacing: 12
        rowSpacing: 6

        Label { text: qsTr("Name"); color: Theme.text2 }
        TemplateSettingsPane.TemplateTextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: qsTr("Meeting type name, for example Customer Status")
            text: pane.templateModel ? pane.templateModel.draftName : ""
            onTextEdited: pane.templateModel.draftName = text
        }

        Label { text: qsTr("Duration"); color: Theme.text2 }
        RowLayout {
            Layout.fillWidth: true
            StyledComboBox {
                id: duration
                readonly property var options: [
                    { value: 15, label: qsTr("15 minutes") },
                    { value: 30, label: qsTr("30 minutes") },
                    { value: 45, label: qsTr("45 minutes") },
                    { value: 60, label: qsTr("1 hour") },
                    { value: 90, label: qsTr("1.5 hours") },
                    { value: 120, label: qsTr("2 hours") },
                    { value: 180, label: qsTr("3 hours") },
                    { value: 240, label: qsTr("4 hours") }
                ]
                Layout.preferredWidth: 140
                model: options
                textRole: "label"
                valueRole: "value"
                currentIndex: pane.templateModel
                              ? pane._indexOf(options, pane.templateModel.draftDurationMinutes) : -1
                onActivated: pane.templateModel.draftDurationMinutes = currentValue
            }
            Item { Layout.fillWidth: true }
        }

        Label { text: qsTr("Subject"); color: Theme.text2 }
        TemplateSettingsPane.TemplateTextField {
            id: subject
            Layout.fillWidth: true
            placeholderText: qsTr("{{ project.number }} {{ project.name }} Status Meeting")
            text: pane.templateModel ? pane.templateModel.draftSubject : ""
            spellCheckEnabled: true
            insertFields: pane.templateModel ? pane.templateModel.availableFields : []
            onTextEdited: pane.templateModel.draftSubject = text
            onInsertFieldRequested: (path) => pane.insertField(subject, path, "subject")
        }
    }

    Label { visible: pane._editing; text: qsTr("Agenda"); color: Theme.text2 }
    RichTextEditor {
        id: richAgenda
        visible: pane._editing
        minimumHeight: 160
        placeholderText: qsTr("Agenda placed in the body of the meeting invitation")
        value: pane.templateModel ? pane.templateModel.draftRichAgenda : ""
        editingEnabled: pane._editing
        insertFields: pane.templateModel ? pane.templateModel.availableFields : []
        onValueEdited: if (pane.templateModel) pane.templateModel.draftRichAgenda = value
        onInsertFieldRequested: (path) => pane.insertField(richAgenda.editor, path, "richAgenda")
    }
    Label {
        Layout.fillWidth: true
        visible: pane.templateModel && pane.templateModel.diagnostic !== ""
        text: pane.templateModel ? pane.templateModel.diagnostic : ""
        color: Theme.red
        wrapMode: Text.WordWrap
    }

    ThemedMessageDialog {
        id: validationDialog
        objectName: "meetingValidationDialog"
        title: qsTr("Meeting type needs attention")
    }
    ThemedMessageDialog {
        id: deleteDialog
        title: qsTr("Delete Meeting Type")
        iconName: "delete"
        confirmText: qsTr("Delete")
        destructive: true
        onConfirmed: if (pane.templateModel) pane.templateModel.deleteSelected()
    }
    ThemedMessageDialog {
        id: resetDialog
        title: qsTr("Reset Meeting Templates")
        message: qsTr("Replace all meeting templates, including custom types and edits, with the defaults?")
        iconName: "warning"
        confirmText: qsTr("Reset")
        destructive: true
        onConfirmed: if (pane.templateModel) pane.templateModel.resetDefaults()
    }
}

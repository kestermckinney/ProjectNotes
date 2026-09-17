// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
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
    component TemplateComboBox: ComboBox {
        id: control
        implicitHeight: 32
        font.pixelSize: Theme.fontBody
        contentItem: Text {
            text: control.displayText
            color: Theme.text
            font: control.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            anchors.fill: parent
            anchors.leftMargin: 9
            anchors.rightMargin: 28
        }
        background: Rectangle {
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: control.activeFocus ? Theme.accent : Theme.border
        }
        indicator: MaterialIcon {
            name: "expand_more"
            size: 16
            color: Theme.text3
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 7
        }
        popup: Popup {
            y: control.height + 2
            width: control.width
            implicitHeight: Math.min(contentItem.implicitHeight + 2, 280)
            padding: 1
            background: Rectangle {
                radius: Theme.radiusSm
                color: Theme.raise
                border.color: Theme.border
            }
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: control.popup.visible ? control.delegateModel : null
                ScrollIndicator.vertical: ScrollIndicator {}
            }
        }
        delegate: ItemDelegate {
            id: itemDelegate
            // `index` is a delegate context property, not an ItemDelegate
            // property.  Declare it so every popup row receives its own model
            // index; accessing `itemDelegate.index` instead resolved to an
            // undefined value and made every row display the first option.
            required property int index
            width: control.width
            contentItem: Text {
                text: control.textAt(itemDelegate.index)
                color: Theme.text
                font.pixelSize: Theme.fontBody
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: itemDelegate.highlighted ? Theme.accentSoft : "transparent"
            }
            highlighted: control.highlightedIndex === itemDelegate.index
            onClicked: {
                control.currentIndex = itemDelegate.index
                control.activated(itemDelegate.index)
                control.popup.close()
            }
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
        else if (target === "plainBody")
            pane.templateModel.draftPlainBody = editor.text
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
        templateValidationDialog.text = pane.templateModel.diagnostic !== ""
                ? pane.templateModel.diagnostic
                : qsTr("The template could not be saved or validated.")
        templateValidationDialog.open()
        return false
    }

    Label {
        text: qsTr("Email templates")
        color: Theme.text
        font.pixelSize: Theme.fontXl
        font.weight: Font.DemiBold
    }
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("Preferred email backend"); color: Theme.text2 }
        TemplateComboBox {
            Layout.preferredWidth: 250
            model: [
                { value: "mailto", label: qsTr("Default client (plain text)") },
                { value: "thunderbird", label: qsTr("Thunderbird") },
                { value: "graph", label: qsTr("Microsoft 365 draft") }
            ]
            textRole: "label"
            valueRole: "value"
            currentIndex: {
                for (let i = 0; i < model.length; ++i)
                    if (model[i].value === DesktopAppController.preferredEmailBackend) return i
                return 0
            }
            onActivated: DesktopAppController.preferredEmailBackend = currentValue
        }
        Item { Layout.fillWidth: true }
    }
    TextField {
        Layout.fillWidth: true
        visible: DesktopAppController.preferredEmailBackend === "thunderbird"
        placeholderText: qsTr("Thunderbird executable path")
        text: DesktopAppController.thunderbirdExecutable
        onEditingFinished: DesktopAppController.thunderbirdExecutable = text
    }
    Label {
        Layout.fillWidth: true
        text: DesktopAppController.preferredEmailBackend === "graph"
            ? qsTr("Microsoft 365 can create a draft after separate draft consent and verified account identity. It never sends email automatically.")
            : qsTr("This preference is retained here. Its review action appears only when the selected adapter supports the prepared content.")
        color: Theme.text3
        font.pixelSize: Theme.fontSm
        wrapMode: Text.WordWrap
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
        TemplateComboBox {
            id: workflow
            Layout.preferredWidth: 220
            model: [
                { value: "send-meeting-notes", label: qsTr("Send Meeting Notes") },
                { value: "tracker-items-report", label: qsTr("Tracker Items Report") },
                { value: "status-report", label: qsTr("Status Report") },
                { value: "meeting-notes-report", label: qsTr("Meeting Notes Report") }
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
            TextField {
                Layout.fillWidth: true
                text: pane.templateModel ? pane.templateModel.draftName : ""
                enabled: pane.templateModel !== null
                onTextEdited: pane.templateModel.draftName = text
            }
            Label { text: qsTr("Subject"); color: Theme.text2 }
            TextField {
                id: subject
                Layout.fillWidth: true
                text: pane.templateModel ? pane.templateModel.draftSubject : ""
                enabled: pane.templateModel !== null
                onTextEdited: pane.templateModel.draftSubject = text
                SpellCheckField {
                    insertFields: pane.templateModel ? pane.templateModel.availableFields : []
                    onInsertFieldRequested: (path) => pane.insertField(subject, path, "subject")
                }
            }
        }
    }

    Label { text: qsTr("Rich body"); color: Theme.text2 }
    Label { text: qsTr("Include exactly one {{ content.body }} protected content block. It preserves native report content while you add prose around it."); color: Theme.text3; font.pixelSize: Theme.fontSm; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    RichTextEditor {
        id: richBody
        minimumHeight: 120
        placeholderText: qsTr("Rich HTML template prose")
        value: pane.templateModel ? pane.templateModel.draftRichBody : ""
        editingEnabled: pane.templateModel !== null
        insertFields: pane.templateModel ? pane.templateModel.availableFields : []
        onValueEdited: if (pane.templateModel) pane.templateModel.draftRichBody = value
        onInsertFieldRequested: (path) => pane.insertField(richBody.editor, path, "richBody")
    }
    Label { text: qsTr("Plain-text body (optional)"); color: Theme.text2 }
    TextArea {
        id: plainBody
        Layout.fillWidth: true
        Layout.preferredHeight: 90
        wrapMode: TextArea.Wrap
        placeholderText: qsTr("Plain-text alternative")
        text: pane.templateModel ? pane.templateModel.draftPlainBody : ""
        enabled: pane.templateModel !== null
        onTextChanged: if (activeFocus && pane.templateModel) pane.templateModel.draftPlainBody = text
        SpellCheckField {
            insertFields: pane.templateModel ? pane.templateModel.availableFields : []
            onInsertFieldRequested: (path) => pane.insertField(plainBody, path, "plainBody")
        }
    }
    Label {
        Layout.fillWidth: true
        visible: pane.templateModel && pane.templateModel.diagnostic !== ""
        text: pane.templateModel ? pane.templateModel.diagnostic : ""
        color: Theme.red
        wrapMode: Text.WordWrap
    }
    RowLayout {
        Button { primary: true; text: qsTr("Save template"); enabled: pane.templateModel !== null; onClicked: pane.templateModel.save() }
        Item { Layout.fillWidth: true }
    }

    // Qt Quick Dialogs uses the platform's standard message dialog where one
    // is available, rather than leaving a validation message easy to miss in
    // the page chrome.
    MessageDialog {
        id: templateValidationDialog
        title: qsTr("Email template needs attention")
        buttons: MessageDialog.Ok
    }
}

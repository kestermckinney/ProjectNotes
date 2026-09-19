// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// View-only companion for RecipientSelectionModel. The controller owns model
// lifetime and validation; this pane only invokes its narrowly exposed edits.
ColumnLayout {
    id: pane
    property var recipientModel: null
    // The review controller emits state changes when asynchronous snapshot
    // preparation completes; use it to load presets after the source exists.
    property var reviewController: null
    // DesktopAppController owns the immutable snapshot and applies this rule
    // through the C++ resolver; the pane never filters people itself.
    property var audienceController: null
    // Report delivery starts with a compact, mockup-like recipient checklist.
    // The full rule/preset/manual-address surface remains available on demand.
    property bool compact: false
    // Project reports always use their fixed project-team audience.  They do
    // not expose alternate audience rules, saved audiences, or manual
    // additions that could introduce people outside that team.
    property bool fixedAudience: false
    // A workflow may supply a fixed, preselected distribution while still
    // allowing its recipients to be reviewed individually.
    property bool recipientListOnly: false
    property bool advancedOpen: false
    spacing: 8

    component StandardCheckBox: CheckBox {
        id: standardCheckBox
        indicator: Rectangle {
            implicitWidth: 16
            implicitHeight: 16
            radius: 4
            x: standardCheckBox.leftPadding
            y: standardCheckBox.height / 2 - height / 2
            color: standardCheckBox.checked ? Theme.accent : Theme.surface
            border.color: standardCheckBox.checked ? Theme.accent : Theme.border
            MaterialIcon {
                anchors.centerIn: parent
                visible: standardCheckBox.checked
                name: "check"
                size: 12
                color: "white"
            }
        }
        contentItem: Text {
            text: standardCheckBox.text
            color: Theme.text
            font.pixelSize: Theme.fontBody
            leftPadding: standardCheckBox.indicator.width + 7
            verticalAlignment: Text.AlignVCenter
        }
    }

    Label { text: qsTr("Recipients"); color: Theme.text; font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold }
    Label {
        text: pane.recipientModel
            ? qsTr("%1 of %2 recipient(s) selected").arg(pane.recipientModel.selectedRecipientCount).arg(pane.recipientModel.recipientCount)
            : qsTr("No recipients loaded")
        color: Theme.text2
    }
    Label {
        visible: pane.recipientModel !== null
        text: pane.recipientModel
            ? qsTr("To: %1  ·  Cc: %2  ·  Bcc: %3")
                  .arg(pane.recipientModel.toRecipientCount)
                  .arg(pane.recipientModel.ccRecipientCount)
                  .arg(pane.recipientModel.bccRecipientCount)
            : ""
        color: Theme.text3
        font.pixelSize: Theme.fontSm
    }
    Label {
        visible: pane.recipientModel && pane.recipientModel.audienceDiagnostic.length > 0
        Layout.fillWidth: true
        text: pane.recipientModel ? pane.recipientModel.audienceDiagnostic : ""
        color: Theme.amber
        wrapMode: Text.Wrap
    }
    Label {
        visible: pane.recipientModel && pane.recipientModel.internalAudienceWarning.length > 0
        Layout.fillWidth: true
        text: pane.recipientModel ? pane.recipientModel.internalAudienceWarning : ""
        color: Theme.amber
        wrapMode: Text.Wrap
    }
    RowLayout {
        Layout.fillWidth: true
        visible: pane.compact && !pane.recipientListOnly && pane.recipientModel !== null
        Button {
            text: qsTr("Select all")
            onClicked: pane.recipientModel.selectAll()
        }
        Button {
            text: qsTr("Clear")
            onClicked: pane.recipientModel.clearSelection()
        }
        Item { Layout.fillWidth: true }
        Button {
            visible: !pane.fixedAudience
            text: pane.advancedOpen ? qsTr("Hide advanced") : qsTr("Advanced")
            onClicked: pane.advancedOpen = !pane.advancedOpen
        }
    }
    Connections {
        target: pane.reviewController
        function onStateChanged() { presetRow.refresh() }
    }

    RowLayout {
        id: presetRow
        Layout.fillWidth: true
        visible: !pane.fixedAudience && pane.audienceController !== null && (!pane.compact || pane.advancedOpen)
        property var presets: []
        function refresh(selectName) {
            presetChoice.selectedId = ""
            presetChoice.value = ""
            presets = pane.audienceController ? pane.audienceController.reviewAudiencePresets() : []
            if (!selectName) return
            for (var i = 0; i < presets.length; ++i)
                if (presets[i].name === selectName) {
                    presetChoice.selectedId = presets[i].id
                    presetChoice.value = presets[i].name
                    return
                }
        }
        Component.onCompleted: refresh()
        ComboField {
            id: presetChoice
            Layout.fillWidth: true
            property string selectedId: ""
            options: presetRow.presets.map(function(preset) { return preset.name })
            value: ""
            includeNone: true
            noneLabel: qsTr("Saved audience")
            onActivated: function(value) {
                selectedId = ""
                for (var i = 0; i < presetRow.presets.length; ++i)
                    if (presetRow.presets[i].name === value) {
                        selectedId = presetRow.presets[i].id
                        break
                    }
            }
        }
        Button {
            text: qsTr("Apply saved")
            enabled: presetChoice.selectedId !== ""
            onClicked: if (pane.audienceController) pane.audienceController.applyReviewAudiencePreset(presetChoice.selectedId)
        }
        Button {
            text: qsTr("Set project default")
            enabled: presetChoice.selectedId !== ""
            onClicked: if (pane.audienceController && pane.audienceController.setReviewAudiencePresetDefault(presetChoice.selectedId)) presetRow.refresh()
        }
    }
    RowLayout {
        Layout.fillWidth: true
        visible: !pane.fixedAudience && pane.audienceController !== null && (!pane.compact || pane.advancedOpen)
        FormField { id: presetName; Layout.fillWidth: true; placeholder: qsTr("Save audience preset") }
        Button {
            text: qsTr("Save audience")
            enabled: presetName.text.trim().length > 0
            onClicked: if (pane.audienceController && pane.audienceController.saveReviewAudiencePreset(presetName.text)) {
                var savedName = presetName.text.trim()
                presetName.clear()
                presetRow.refresh(savedName)
            }
        }
    }

    FormField {
        id: recipientSearch
        Layout.fillWidth: true
        placeholder: qsTr("Search selected audience")
        visible: !pane.recipientListOnly
    }
    ListView {
        id: recipientList
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(220, Math.max(contentHeight, count > 0 ? 80 : 0))
        clip: true
        model: pane.recipientModel
        spacing: 4
        section.property: "companyGroup"
        section.criteria: ViewSection.FullString
        section.delegate: Label {
            required property string section
            width: recipientList.width
            text: section
            color: Theme.text2
            font.weight: Font.DemiBold
            topPadding: 6
        }
        delegate: RowLayout {
            id: recipientRow
            readonly property var recipient: model
            width: recipientList.width
            readonly property bool matchesSearch: recipientSearch.text.trim() === ""
                || (model.name + " " + model.address + " " + model.companyName + " " + model.sourceReason)
                       .toLocaleLowerCase().indexOf(recipientSearch.text.trim().toLocaleLowerCase()) >= 0
            visible: matchesSearch
            height: matchesSearch ? implicitHeight : 0
            spacing: 8
            StandardCheckBox {
                id: recipientCheck
                checked: model.selected
                Accessible.name: qsTr("Include %1").arg(model.name)
                onToggled: if (pane.recipientModel) pane.recipientModel.setSelected(model.personId, checked)
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Label { text: model.name; color: Theme.text; elide: Text.ElideRight; Layout.fillWidth: true }
                Label { text: model.address + (model.companyName ? " · " + model.companyName : ""); color: Theme.text2; elide: Text.ElideRight; Layout.fillWidth: true; font.pixelSize: Theme.fontSm }
                Label { text: model.sourceReason; color: Theme.text3; elide: Text.ElideRight; Layout.fillWidth: true; font.pixelSize: Theme.fontXs }
            }
            ComboField {
                visible: !pane.recipientListOnly
                Layout.preferredWidth: 82
                Layout.fillWidth: false
                options: [qsTr("To"), qsTr("Cc"), qsTr("Bcc")]
                value: options[recipientRow.recipient.recipientRole]
                enabled: recipientRow.recipient.selected
                Accessible.name: qsTr("Recipient role for %1").arg(recipientRow.recipient.name)
                onActivated: function(value) {
                    if (pane.recipientModel)
                        pane.recipientModel.setRecipientRoleValue(recipientRow.recipient.personId,
                                                                  options.indexOf(value))
                }
            }
            Label {
                visible: pane.recipientListOnly
                text: [qsTr("To"), qsTr("Cc"), qsTr("Bcc")][model.recipientRole]
                color: Theme.text2
            }
            Button {
                visible: model.manual
                text: qsTr("Remove")
                Accessible.name: qsTr("Remove %1").arg(model.name)
                onClicked: if (pane.recipientModel) pane.recipientModel.removeManual(model.personId)
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        visible: !pane.fixedAudience && (!pane.compact || pane.advancedOpen)
        FormField { id: manualName; Layout.preferredWidth: 150; Layout.fillWidth: false; placeholder: qsTr("Name") }
        FormField { id: manualAddress; Layout.fillWidth: true; placeholder: qsTr("email@example.com"); inputMethodHints: Qt.ImhEmailCharactersOnly }
        Button {
            text: qsTr("Add")
            enabled: manualAddress.text.trim().length > 0
            onClicked: if (pane.recipientModel && pane.recipientModel.addManual(manualName.text, manualAddress.text)) { manualName.clear(); manualAddress.clear() }
        }
    }
    Button {
        visible: !pane.compact || pane.advancedOpen
        text: qsTr("Reset recipients")
        enabled: pane.recipientModel !== null
        onClicked: if (pane.recipientModel) pane.recipientModel.reset()
    }
}

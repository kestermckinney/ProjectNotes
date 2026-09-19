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
    // Report delivery uses a compact recipient checklist with bulk
    // select/clear actions; saved-audience controls are always shown.
    property bool compact: false
    // Send Meeting Notes uses a fixed attendee distribution and does not
    // expose saved audiences.  Recipients only ever come from people records.
    property bool fixedAudience: false
    // A workflow may supply a fixed, preselected distribution while still
    // allowing its recipients to be reviewed individually.
    property bool recipientListOnly: false
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
    }
    Connections {
        target: pane.reviewController
        function onStateChanged() { presetRow.refresh() }
    }
    Connections {
        target: pane.audienceController
        ignoreUnknownSignals: true
        function onReviewAudiencePresetChanged() { presetRow.refresh() }
    }

    RowLayout {
        id: presetRow
        Layout.fillWidth: true
        visible: !pane.fixedAudience && pane.audienceController !== null
        property var presets: []
        // Audience last copied into the save-name box; refreshes that keep the
        // same selection leave whatever the user has typed alone.
        property string namedId: ""
        property string namedText: ""
        function showName(preset) {
            var id = preset ? preset.id : ""
            if (id === namedId) return
            if (preset)
                presetName.text = preset.name
            else if (presetName.text === namedText)
                presetName.clear()
            namedId = id
            namedText = preset ? preset.name : ""
        }
        // Saved audiences are shared by every report of the project; the one
        // this report loads automatically is marked in the list, and whichever
        // audience is loaded into the review is shown as selected.
        function label(preset) {
            return preset.reportDefault ? qsTr("%1 (report default)").arg(preset.name) : preset.name
        }
        function refresh(selectId, selectName) {
            presetChoice.selectedId = ""
            presetChoice.value = ""
            presets = pane.audienceController ? pane.audienceController.reviewAudiencePresets() : []
            for (var i = 0; i < presets.length; ++i)
                if ((selectId && presets[i].id === selectId) || (selectName && presets[i].name === selectName)
                        || (!selectId && !selectName && presets[i].applied)) {
                    presetChoice.selectedId = presets[i].id
                    presetChoice.value = label(presets[i])
                    showName(presets[i])
                    return
                }
            showName(null)
        }
        Component.onCompleted: refresh()
        ComboField {
            id: presetChoice
            Layout.fillWidth: true
            property string selectedId: ""
            options: presetRow.presets.map(function(preset) { return presetRow.label(preset) })
            value: ""
            includeNone: true
            noneLabel: qsTr("Saved audience")
            onActivated: function(value) {
                selectedId = ""
                for (var i = 0; i < presetRow.presets.length; ++i)
                    if (presetRow.label(presetRow.presets[i]) === value) {
                        selectedId = presetRow.presets[i].id
                        break
                    }
                // Choosing a saved audience applies it immediately; the
                // placeholder leaves the current selection untouched.
                if (selectedId !== "" && pane.audienceController)
                    pane.audienceController.applyReviewAudiencePreset(selectedId)
            }
        }
        Button {
            text: qsTr("Set as report default")
            enabled: presetChoice.selectedId !== ""
            onClicked: {
                var chosenId = presetChoice.selectedId
                if (pane.audienceController && pane.audienceController.setReviewAudiencePresetDefault(chosenId))
                    presetRow.refresh(chosenId)
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        visible: !pane.fixedAudience && pane.audienceController !== null
        FormField { id: presetName; Layout.fillWidth: true; placeholder: qsTr("Save audience preset") }
        Button {
            text: qsTr("Save audience")
            enabled: presetName.text.trim().length > 0
            onClicked: if (pane.audienceController && pane.audienceController.saveReviewAudiencePreset(presetName.text)) {
                // The saved audience stays selected with its name in the box,
                // ready to be edited and saved over again.
                presetRow.refresh("", presetName.text.trim())
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
        }
    }

    Button {
        visible: !pane.recipientListOnly
        text: qsTr("Reset recipients")
        enabled: pane.recipientModel !== null
        onClicked: if (pane.recipientModel) pane.recipientModel.reset()
    }
}

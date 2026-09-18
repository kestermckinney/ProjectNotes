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
    property string defaultPeopleSource: "project-team"
    property string defaultCompanyFilter: "all"
    property bool defaultIncludeUnknownCompany: false
    property bool defaultExcludeProjectManager: false
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
    property var companyChoices: []
    property var selectedCompanyIds: []
    property var peopleChoices: []
    property var chosenPersonIds: []
    spacing: 8

    function refreshCompanies() {
        companyChoices = pane.audienceController ? pane.audienceController.reviewAudienceCompanies() : []
        peopleChoices = pane.audienceController ? pane.audienceController.reviewAudiencePeople() : []
        selectedCompanyIds = []
        chosenPersonIds = []
    }
    function setCompanySelected(companyId, selected) {
        var ids = selectedCompanyIds.slice()
        var index = ids.indexOf(companyId)
        if (selected && index < 0) ids.push(companyId)
        else if (!selected && index >= 0) ids.splice(index, 1)
        selectedCompanyIds = ids
    }
    function setPersonSelected(personId, selected) {
        var ids = chosenPersonIds.slice()
        var index = ids.indexOf(personId)
        if (selected && index < 0) ids.push(personId)
        else if (!selected && index >= 0) ids.splice(index, 1)
        chosenPersonIds = ids
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
    GridLayout {
        columns: 2
        Layout.fillWidth: true
        visible: !pane.fixedAudience && pane.audienceController !== null && (!pane.compact || pane.advancedOpen)
        columnSpacing: 8
        rowSpacing: 4
        Label { text: qsTr("Audience source"); color: Theme.text2 }
        ComboBox {
            id: peopleSource
            Layout.fillWidth: true
            textRole: "label"
            valueRole: "value"
            model: [
                { label: qsTr("Project team"), value: "project-team" },
                { label: qsTr("Meeting attendees"), value: "meeting-attendees" },
                { label: qsTr("Status recipients"), value: "status-recipients" },
                { label: qsTr("Current selection"), value: "current-selection" },
                { label: qsTr("Choose people"), value: "chosen-people" }
            ]
            currentIndex: indexOfValue(pane.defaultPeopleSource)
        }
        Label { text: qsTr("Company filter"); color: Theme.text2 }
        ComboBox {
            id: companyFilter
            Layout.fillWidth: true
            textRole: "label"
            valueRole: "value"
            model: [
                { label: qsTr("All companies"), value: "all" },
                { label: qsTr("Managing company"), value: "managing-company" },
                { label: qsTr("Project client"), value: "project-client" },
                { label: qsTr("Except project client"), value: "except-project-client" },
                { label: qsTr("Selected companies"), value: "selected-companies" }
            ]
            currentIndex: indexOfValue(pane.defaultCompanyFilter)
        }
    }
    ColumnLayout {
        Layout.fillWidth: true
        visible: !pane.fixedAudience && (!pane.compact || pane.advancedOpen) && companyFilter.currentValue === "selected-companies"
        spacing: 2
        Label { text: qsTr("Choose companies"); color: Theme.text2; font.weight: Font.DemiBold }
        Repeater {
            model: pane.companyChoices
            delegate: CheckBox {
                required property var modelData
                text: qsTr("%1 (%2 people)").arg(modelData.name).arg(modelData.peopleCount)
                checked: pane.selectedCompanyIds.indexOf(modelData.id) >= 0
                onToggled: pane.setCompanySelected(modelData.id, checked)
            }
        }
    }
    ColumnLayout {
        Layout.fillWidth: true
        visible: !pane.fixedAudience && (!pane.compact || pane.advancedOpen) && peopleSource.currentValue === "chosen-people"
        spacing: 2
        Label { text: qsTr("Choose people"); color: Theme.text2; font.weight: Font.DemiBold }
        TextField {
            id: choosePeopleSearch
            Layout.fillWidth: true
            placeholderText: qsTr("Search people")
        }
        Repeater {
            model: pane.peopleChoices
            delegate: CheckBox {
                required property var modelData
                text: modelData.name + (modelData.companyName ? " · " + modelData.companyName : "")
                checked: pane.chosenPersonIds.indexOf(modelData.id) >= 0
                visible: choosePeopleSearch.text.trim() === ""
                    || (modelData.name + " " + modelData.address + " " + modelData.companyName)
                           .toLocaleLowerCase().indexOf(choosePeopleSearch.text.trim().toLocaleLowerCase()) >= 0
                onToggled: pane.setPersonSelected(modelData.id, checked)
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        visible: !pane.fixedAudience && pane.audienceController !== null && (!pane.compact || pane.advancedOpen)
        CheckBox { id: includeUnknown; text: qsTr("Include unknown company"); checked: pane.defaultIncludeUnknownCompany }
        CheckBox { id: excludeManager; text: qsTr("Exclude project manager"); checked: pane.defaultExcludeProjectManager }
        Item { Layout.fillWidth: true }
        Button {
            text: qsTr("Apply audience")
            onClicked: if (pane.audienceController)
                pane.audienceController.applyReviewAudienceRuleAdvanced(peopleSource.currentValue,
                                                                         companyFilter.currentValue,
                                                                         pane.selectedCompanyIds,
                                                                         pane.chosenPersonIds,
                                                                         includeUnknown.checked,
                                                                         excludeManager.checked)
        }
    }

    Connections {
        target: pane.reviewController
        function onStateChanged() { presetRow.refresh(); pane.refreshCompanies() }
    }
    Component.onCompleted: refreshCompanies()

    RowLayout {
        id: presetRow
        Layout.fillWidth: true
        visible: !pane.fixedAudience && pane.audienceController !== null && (!pane.compact || pane.advancedOpen)
        property var presets: []
        function refresh() {
            presets = pane.audienceController ? pane.audienceController.reviewAudiencePresets() : []
        }
        Component.onCompleted: refresh()
        ComboBox {
            id: presetChoice
            Layout.fillWidth: true
            textRole: "name"
            valueRole: "id"
            model: presetRow.presets
            displayText: currentIndex >= 0 ? currentText : qsTr("Saved audience")
        }
        Button {
            text: qsTr("Apply saved")
            enabled: presetChoice.currentIndex >= 0
            onClicked: if (pane.audienceController) pane.audienceController.applyReviewAudiencePreset(presetChoice.currentValue)
        }
        Button {
            text: projectPreset.checked ? qsTr("Set project default") : qsTr("Set default")
            enabled: presetChoice.currentIndex >= 0
            onClicked: if (pane.audienceController && pane.audienceController.setReviewAudiencePresetDefault(presetChoice.currentValue, projectPreset.checked)) presetRow.refresh()
        }
    }
    RowLayout {
        Layout.fillWidth: true
        visible: !pane.fixedAudience && pane.audienceController !== null && (!pane.compact || pane.advancedOpen)
        TextField { id: presetName; Layout.fillWidth: true; placeholderText: qsTr("Save audience preset") }
        CheckBox { id: projectPreset; text: qsTr("This project") }
        Button {
            text: qsTr("Save audience")
            enabled: presetName.text.trim().length > 0
            onClicked: if (pane.audienceController && pane.audienceController.saveReviewAudiencePreset(presetName.text, projectPreset.checked)) {
                presetName.clear()
                presetRow.refresh()
            }
        }
    }

    TextField {
        id: recipientSearch
        Layout.fillWidth: true
        placeholderText: qsTr("Search selected audience")
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
            CheckBox {
                id: recipientCheck
                checked: model.selected
                Accessible.name: qsTr("Include %1").arg(model.name)
                onToggled: if (pane.recipientModel) pane.recipientModel.setSelected(model.personId, checked)
                indicator: Rectangle {
                    implicitWidth: 16; implicitHeight: 16
                    radius: 4
                    x: recipientCheck.leftPadding
                    y: recipientCheck.height / 2 - height / 2
                    color: recipientCheck.checked ? Theme.accent : Theme.surface
                    border.color: recipientCheck.checked ? Theme.accent : Theme.border
                    MaterialIcon {
                        anchors.centerIn: parent
                        visible: recipientCheck.checked
                        name: "check"; size: 12; color: "white"
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Label { text: model.name; color: Theme.text; elide: Text.ElideRight; Layout.fillWidth: true }
                Label { text: model.address + (model.companyName ? " · " + model.companyName : ""); color: Theme.text2; elide: Text.ElideRight; Layout.fillWidth: true; font.pixelSize: Theme.fontSm }
                Label { text: model.sourceReason; color: Theme.text3; elide: Text.ElideRight; Layout.fillWidth: true; font.pixelSize: Theme.fontXs }
            }
            ComboBox {
                visible: !pane.recipientListOnly
                model: [qsTr("To"), qsTr("Cc"), qsTr("Bcc")]
                currentIndex: recipientRow.recipient.recipientRole
                enabled: recipientRow.recipient.selected
                Accessible.name: qsTr("Recipient role for %1").arg(recipientRow.recipient.name)
                onActivated: if (pane.recipientModel) pane.recipientModel.setRecipientRoleValue(recipientRow.recipient.personId, currentIndex)
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
        TextField { id: manualName; Layout.preferredWidth: 150; placeholderText: qsTr("Name") }
        TextField { id: manualAddress; Layout.fillWidth: true; placeholderText: qsTr("email@example.com"); inputMethodHints: Qt.ImhEmailCharactersOnly }
        Button {
            text: qsTr("Add")
            enabled: manualAddress.text.trim().length > 0
            onClicked: if (pane.recipientModel && pane.recipientModel.addManual(manualName.text, manualAddress.text)) { manualName.clear(); manualAddress.clear() }
        }
    }
    CheckBox {
        visible: !pane.fixedAudience && (!pane.compact || pane.advancedOpen)
        text: qsTr("Add recipients in the email client instead")
        checked: pane.recipientModel ? pane.recipientModel.addressLaterExplicitlyChosen : false
        onToggled: if (pane.recipientModel) pane.recipientModel.setAddressLaterExplicitlyChosen(checked)
    }
    CheckBox {
        id: showExcluded
        visible: !pane.fixedAudience && (!pane.compact || pane.advancedOpen) && pane.recipientModel && pane.recipientModel.excludedRecipients.length > 0
        text: pane.recipientModel
            ? qsTr("Show %1 contact(s) not included").arg(pane.recipientModel.excludedRecipients.length)
            : ""
    }
    Repeater {
        model: !pane.fixedAudience && (!pane.compact || pane.advancedOpen) && showExcluded.checked && pane.recipientModel ? pane.recipientModel.excludedRecipients : []
        delegate: ColumnLayout {
            required property var modelData
            Layout.fillWidth: true
            spacing: 0
            Label {
                Layout.fillWidth: true
                text: modelData.name + (modelData.companyName ? " · " + modelData.companyName : "")
                color: Theme.text2
                elide: Text.ElideRight
            }
            Label {
                Layout.fillWidth: true
                text: modelData.reason
                color: Theme.text3
                font.pixelSize: Theme.fontXs
                wrapMode: Text.Wrap
            }
        }
    }
    Button {
        visible: !pane.compact || pane.advancedOpen
        text: qsTr("Reset recipients")
        enabled: pane.recipientModel !== null
        onClicked: if (pane.recipientModel) pane.recipientModel.reset()
    }
}

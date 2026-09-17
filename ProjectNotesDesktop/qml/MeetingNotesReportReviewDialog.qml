// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Report configuration dialog. Snapshot construction and report filtering
// remain in DesktopAppController/native services.
Dialog {
    id: dialog
    modal: true
    // Keep this dialog centered in the application window even when it is
    // declared inside a narrow container such as ProjectSidebar.
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(460, parent ? parent.width - 36 : 460)
    height: Math.min(570, parent ? parent.height - 36 : 570)
    padding: 0
    property string workflow: "meeting-notes-report"
    title: workflow === "status-report" ? qsTr("Generate Status Report")
          : workflow === "tracker-items-report" ? qsTr("Generate Tracker Items Report")
          : qsTr("Generate Meeting Notes Report")
    property var controller: null
    property var recipientModel: null
    property var templateModel: null
    property string projectId: ""
    property string emailMode: "inline-html"
    property bool retainHtml: false
    property bool displayPdf: false
    readonly property var emailModeOptions: [
        { label: qsTr("Inline HTML"), value: "inline-html" },
        { label: qsTr("HTML attachment"), value: "html-attachment" },
        { label: qsTr("PDF attachment"), value: "pdf-attachment" },
        { label: qsTr("Do not email"), value: "none" }
    ]
    readonly property var office365: DesktopAppController.office365SettingsModel
    readonly property bool graphDraftReady: DesktopAppController.preferredEmailBackend === "graph"
                                         && office365 && office365.emailDraftsGranted
                                         && office365.accountLabel !== ""

    // Match the checkbox treatment used by the note editor: a compact rounded
    // indicator with the application accent and Material check mark.
    component ReportCheckBox: CheckBox {
        id: checkBox
        indicator: Rectangle {
            implicitWidth: 16
            implicitHeight: 16
            radius: 4
            x: checkBox.leftPadding
            y: parent.height / 2 - height / 2
            color: checkBox.checked ? Theme.accent : Theme.surface
            border.color: checkBox.checked ? Theme.accent : Theme.border
            MaterialIcon {
                anchors.centerIn: parent
                visible: checkBox.checked
                name: "check"
                size: 12
                color: "#ffffff"
            }
        }
        contentItem: Text {
            text: checkBox.text
            color: Theme.text
            font.pixelSize: Theme.fontBody
            leftPadding: checkBox.indicator.width + 7
            verticalAlignment: Text.AlignVCenter
        }
    }

    function todayText() {
        var d = new Date()
        return (d.getMonth() + 1 < 10 ? "0" : "") + (d.getMonth() + 1) + "/"
            + (d.getDate() < 10 ? "0" : "") + d.getDate() + "/" + d.getFullYear()
    }
    function emailModeLabel(mode) {
        for (var i = 0; i < emailModeOptions.length; ++i)
            if (emailModeOptions[i].value === mode) return emailModeOptions[i].label
        return emailModeOptions[0].label
    }
    function emailModeValue(label) {
        for (var i = 0; i < emailModeOptions.length; ++i)
            if (emailModeOptions[i].label === label) return emailModeOptions[i].value
        return emailModeOptions[0].value
    }
    function prepare() {
        if (projectId === "") return
        if (workflow === "status-report")
            DesktopAppController.prepareStatusReportReviewWithOptions(projectId, reportDate.text,
                                                                        internalReport.checked, emailMode, retainHtml, displayPdf)
        else if (workflow === "tracker-items-report")
            DesktopAppController.prepareTrackerReportReviewWithOptions(projectId, reportDate.text, internalReport.checked,
                                                                        emailMode, selectedTrackerTypes(), selectedTrackerStatuses(), retainHtml, displayPdf)
        else
            DesktopAppController.prepareMeetingNotesReportReviewWithOptions(projectId, reportDate.text,
                                                                             internalReport.checked, emailMode, retainHtml, displayPdf)
    }
    function selectedTrackerTypes() {
        var values = []
        if (includeTrackerItems.checked) values.push("Tracker")
        if (includeActionItems.checked) values.push("Action")
        return values
    }
    function selectedTrackerStatuses() {
        var values = []
        if (includeNew.checked) values.push("New")
        if (includeAssigned.checked) values.push("Assigned")
        if (includeResolved.checked) values.push("Resolved")
        if (includeDefered.checked) values.push("Defered")
        if (includeCancelled.checked) values.push("Cancelled")
        return values
    }
    onOpened: {
        if (templateModel) templateModel.workflow = workflow
        if (reportDate.text === "") reportDate.text = todayText()
        prepare()
    }

    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    header: Item {
        implicitHeight: 52
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 10
            Label {
                text: dialog.title
                color: Theme.text
                font.pixelSize: Theme.fontXl
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            ToolButton { text: "×"; onClicked: dialog.close() }
        }
    }
    contentItem: ScrollView {
        id: contentScroll
        clip: true
        padding: 16
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        contentWidth: availableWidth

        ColumnLayout {
            width: contentScroll.availableWidth
            spacing: 10

            DateField {
            id: reportDate
            label: dialog.workflow === "meeting-notes-report"
                ? qsTr("Include meetings through") : qsTr("Reporting date")
            Layout.fillWidth: true
            }

            Label {
            text: qsTr("Report options")
            color: Theme.text
            font.pixelSize: Theme.fontLg
            font.weight: Font.DemiBold
            }
            ReportCheckBox {
            id: internalReport
            text: dialog.workflow === "meeting-notes-report"
                ? qsTr("Include internal meetings") : qsTr("Generate internal report")
            }
            ReportCheckBox {
            text: qsTr("Generate HTML report (temporary)")
            checked: dialog.retainHtml
            onToggled: dialog.retainHtml = checked
            }
            ReportCheckBox {
            text: qsTr("Display report when complete")
            checked: dialog.displayPdf
            onToggled: dialog.displayPdf = checked
            }

            ColumnLayout {
            visible: dialog.workflow === "tracker-items-report"
            Layout.fillWidth: true
            spacing: 4
            Label { text: qsTr("Include"); color: Theme.text2; font.weight: Font.DemiBold }
            ReportCheckBox { id: includeTrackerItems; text: qsTr("Tracker items"); checked: true }
            ReportCheckBox { id: includeActionItems; text: qsTr("Action items") }
            Label { text: qsTr("Status"); color: Theme.text2; font.weight: Font.DemiBold }
            ReportCheckBox { id: includeNew; text: qsTr("New"); checked: true }
            ReportCheckBox { id: includeAssigned; text: qsTr("Assigned"); checked: true }
            ReportCheckBox { id: includeResolved; text: qsTr("Resolved") }
            ReportCheckBox { id: includeDefered; text: qsTr("Deferred") }
            ReportCheckBox { id: includeCancelled; text: qsTr("Cancelled") }
            }

            ComboField {
            id: emailModeSelector
            label: qsTr("Email report as")
            options: dialog.emailModeOptions.map(function(option) { return option.label })
            value: dialog.emailModeLabel(dialog.emailMode)
            onActivated: (label) => dialog.emailMode = dialog.emailModeValue(label)
            }

            Label {
            Layout.fillWidth: true
            visible: dialog.controller && dialog.controller.diagnostic !== ""
            text: dialog.controller ? dialog.controller.diagnostic : ""
            color: Theme.red
            wrapMode: Text.Wrap
            }
            RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Generate")
                enabled: !dialog.controller || !dialog.controller.busy
                onClicked: dialog.prepare()
            }
            Button {
                visible: dialog.emailMode === "none"
                text: qsTr("Complete")
                enabled: dialog.controller && dialog.controller.stageName === "reviewing" && !dialog.controller.busy
                onClicked: DesktopAppController.handoffPreparedReview()
            }
            Button {
                visible: dialog.emailMode !== "none" && dialog.graphDraftReady
                text: qsTr("Create Microsoft 365 draft")
                enabled: dialog.controller && dialog.controller.stageName === "reviewing" && !dialog.controller.busy
                onClicked: DesktopAppController.handoffPreparedReview()
            }
            Button {
                visible: dialog.controller && dialog.controller.stageName === "completed"
                         && dialog.controller.presentationUrl.toString() !== ""
                text: qsTr("Open draft in Outlook")
                onClicked: Qt.openUrlExternally(dialog.controller.presentationUrl)
            }
            Button {
                text: qsTr("Cancel")
                onClicked: dialog.close()
            }
            }
        }
    }
}

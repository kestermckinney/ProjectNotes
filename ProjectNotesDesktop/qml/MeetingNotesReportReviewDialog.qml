// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtCore
import QtQuick.Dialogs
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
    title: workflow === "status-report" ? qsTr("Status Report")
          : workflow === "tracker-items-report" ? qsTr("Tracker Items Report")
          : qsTr("Meeting Notes Report")
    property var controller: null
    property var recipientModel: null
    property var templateModel: null
    property string projectId: ""
    property string emailMode: "inline-html"
    property bool openAfterSave: false
    property string pendingSaveUrl: ""
    property string pendingSaveFormat: ""
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
    function defaultReportFileStem() {
        var date = (reportDate.text || "").split("/")
        var datePart = date.length === 3 ? date[2] + date[0] + date[1] : "Report"
        var row = DesktopAppController.projectRowForId(projectId)
        var project = row >= 0 ? DesktopAppController.getProjectData(row) : ({})
        var number = (project.project_number || "").toString().trim()
        var reportName = workflow === "status-report" ? qsTr("Status Report")
                       : workflow === "tracker-items-report" ? qsTr("Tracker Items Report")
                       : qsTr("Meeting Notes Report")
        return [datePart, number, reportName].filter(function(part) { return part !== "" }).join(" ")
    }
    function openSaveDialog() {
        saveReportDialog.currentFile = StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
                + "/" + defaultReportFileStem()
        saveReportDialog.open()
    }
    function generatedExportName(format) {
        if (!controller) return ""
        var suffix = "." + (format || "pdf")
        var names = controller.generatedAttachmentNames
        for (var i = 0; i < names.length; ++i)
            if (names[i].toLowerCase().endsWith(suffix)) return names[i]
        return ""
    }
    function savePendingReport() {
        if (pendingSaveUrl === "" || !controller || controller.busy
                || controller.stageName !== "reviewing") return
        var name = generatedExportName(pendingSaveFormat)
        if (name === "") return
        var destination = pendingSaveUrl
        pendingSaveUrl = ""
        pendingSaveFormat = ""
        if (DesktopAppController.saveReviewGeneratedAttachment(name, destination) && openAfterSave)
            Qt.openUrlExternally(destination)
    }
    function prepare() {
        if (projectId === "") return
        if (workflow === "status-report")
            DesktopAppController.prepareStatusReportReviewWithOptions(projectId, reportDate.text,
                                                                        internalReport.checked, emailMode, false, false)
        else if (workflow === "tracker-items-report")
            DesktopAppController.prepareTrackerReportReviewWithOptions(projectId, reportDate.text, internalReport.checked,
                                                                        emailMode, selectedTrackerTypes(), selectedTrackerStatuses(), false, false)
        else
            DesktopAppController.prepareMeetingNotesReportReviewWithOptions(projectId, reportDate.text,
                                                                             internalReport.checked, emailMode, false, false)
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
    }
    onClosed: {
        pendingSaveUrl = ""
        pendingSaveFormat = ""
    }
    Connections {
        target: dialog.controller
        function onStateChanged() { dialog.savePendingReport() }
    }

    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    // Match the About dialog: an icon-led title row, an explicit close affordance,
    // and a one-pixel divider separating the title from the scrollable content.
    header: null
    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8
            MaterialIcon { name: "description"; size: 20; color: Theme.accent }
            Text {
                text: dialog.title
                color: Theme.text
                font.pixelSize: Theme.font2xl
                font.weight: Font.Bold
                Layout.fillWidth: true
            }
            MaterialIcon {
                name: "close"
                size: 20
                color: Theme.text3
                TapHandler {
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    onTapped: dialog.close()
                }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        ScrollView {
        id: contentScroll
        Layout.fillWidth: true
        Layout.fillHeight: true
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
            Layout.fillWidth: false
            Layout.preferredWidth: 150
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
            text: qsTr("Open after save")
            checked: dialog.openAfterSave
            onToggled: dialog.openAfterSave = checked
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
                text: qsTr("Save As")
                enabled: dialog.projectId !== "" && (!dialog.controller || !dialog.controller.busy)
                onClicked: dialog.openSaveDialog()
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
    FileDialog {
        id: saveReportDialog
        title: qsTr("Save report as")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF files (*.pdf)"), qsTr("HTML files (*.html)")]
        onAccepted: {
            dialog.pendingSaveFormat = selectedNameFilter.extensions.indexOf("html") >= 0 ? "html" : "pdf"
            var destination = selectedFile.toString()
            if (!destination.toLowerCase().endsWith("." + dialog.pendingSaveFormat))
                destination += "." + dialog.pendingSaveFormat
            if (!DesktopAppController.removeExistingReportSaveFile(destination))
                return
            dialog.pendingSaveUrl = destination
            dialog.prepare()
        }
    }
}

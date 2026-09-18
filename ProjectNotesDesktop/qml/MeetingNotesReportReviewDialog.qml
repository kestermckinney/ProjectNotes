// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtCore
import QtQuick.Dialogs
import QtQuick.Layouts
import ProjectNotesDesktop

// A single project-report delivery flow. Report construction, recipient
// resolution, artifact staging, and Graph draft handoff stay native.
Dialog {
    id: dialog
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(720, parent ? parent.width - 36 : 720)
    height: Math.min(760, parent ? parent.height - 36 : 760)
    padding: 0
    title: qsTr("Run report")

    property var controller: null
    property var recipientModel: null
    property var templateModel: null
    property string projectId: ""
    property string workflow: "meeting-notes-report"
    readonly property bool projectReport: workflow === "status-report" || workflow === "tracker-items-report"
    property string delivery: "save"
    property string emailMode: "inline-html"
    property bool internalReport: false
    property bool openAfterSave: false
    property string preparedKey: ""
    property string pendingPreparationKey: ""
    property string pendingSaveUrl: ""
    property string pendingSaveFormat: ""
    property var companyChoices: []
    property var selectedCompanyIds: []

    readonly property var office365: DesktopAppController.office365SettingsModel
    readonly property bool graphDraftReady: DesktopAppController.preferredEmailBackend === "graph"
                                         && office365 && office365.emailDraftsGranted
                                         && office365.accountLabel !== ""
    readonly property bool reviewing: controller && controller.stageName === "reviewing" && !controller.busy
    readonly property bool prepared: reviewing && preparedKey === reportKey(emailMode)
    readonly property var workflowOptions: [
        { label: qsTr("Status Report"), value: "Status Report", workflow: "status-report" },
        { label: qsTr("Tracker Items Report"), value: "Tracker Items", workflow: "tracker-items-report" },
        { label: qsTr("Meeting Notes Report"), value: "Meeting Notes Report", workflow: "meeting-notes-report" }
    ]
    readonly property var emailModeOptions: [
        { label: qsTr("Inline HTML"), value: "inline-html", description: qsTr("Show the report in the email body.") },
        { label: qsTr("HTML attachment"), value: "html-attachment", description: qsTr("Attach a standalone HTML report.") },
        { label: qsTr("PDF attachment"), value: "pdf-attachment", description: qsTr("Attach a print-ready PDF report.") }
    ]

    component ChoiceCard: Rectangle {
        property bool chosen: false
        property string iconName: "description"
        property string heading: ""
        property string detail: ""
        signal selected()
        Layout.fillWidth: true
        implicitHeight: 70
        radius: Theme.radius
        color: chosen ? Theme.accentSoft : Theme.surface
        border.width: chosen ? 2 : 1
        border.color: chosen ? Theme.accent : Theme.border
        RowLayout {
            anchors.fill: parent; anchors.margins: 11; spacing: 10
            Rectangle {
                Layout.preferredWidth: 34; Layout.preferredHeight: 34; radius: Theme.radiusSm
                color: chosen ? Theme.accent : Theme.surface2
                MaterialIcon { anchors.centerIn: parent; name: iconName; size: 18; color: chosen ? "white" : Theme.text2 }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 1
                Label { text: heading; color: chosen ? Theme.accentStrong : Theme.text; font.weight: Font.DemiBold }
                Label { text: detail; color: Theme.text3; font.pixelSize: Theme.fontSm; Layout.fillWidth: true; wrapMode: Text.Wrap }
            }
        }
        TapHandler { onTapped: parent.selected() }
    }

    component CompactCheckBox: CheckBox {
        id: compactCheckBox
        indicator: Rectangle {
            implicitWidth: 16; implicitHeight: 16; radius: 4
            x: compactCheckBox.leftPadding; y: compactCheckBox.height / 2 - height / 2
            color: compactCheckBox.checked ? Theme.accent : Theme.surface
            border.color: compactCheckBox.checked ? Theme.accent : Theme.border
            MaterialIcon { anchors.centerIn: parent; visible: compactCheckBox.checked; name: "check"; size: 12; color: "white" }
        }
        contentItem: Text {
            text: compactCheckBox.text; color: Theme.text; font.pixelSize: Theme.fontBody
            leftPadding: compactCheckBox.indicator.width + 7; verticalAlignment: Text.AlignVCenter
        }
    }

    function todayText() {
        var d = new Date()
        return (d.getMonth() + 1 < 10 ? "0" : "") + (d.getMonth() + 1) + "/"
             + (d.getDate() < 10 ? "0" : "") + d.getDate() + "/" + d.getFullYear()
    }
    function workflowLabel(value) {
        for (var i = 0; i < workflowOptions.length; ++i)
            if (workflowOptions[i].workflow === value) return workflowOptions[i].label
        return workflowOptions[0].label
    }
    function workflowFromOption(value) {
        for (var i = 0; i < workflowOptions.length; ++i)
            if (workflowOptions[i].value === value) return workflowOptions[i].workflow
        return workflowOptions[0].workflow
    }
    function workflowOptionValue(value) {
        for (var i = 0; i < workflowOptions.length; ++i)
            if (workflowOptions[i].workflow === value) return workflowOptions[i].value
        return workflowOptions[0].value
    }
    function modeLabel(value) {
        for (var i = 0; i < emailModeOptions.length; ++i)
            if (emailModeOptions[i].value === value) return emailModeOptions[i].label
        return emailModeOptions[0].label
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
        if (includeDeferred.checked) values.push("Defered")
        if (includeCancelled.checked) values.push("Cancelled")
        return values
    }
    function reportKey(mode) {
        return [workflow, reportDate.text, internalReport, mode,
                selectedTrackerTypes().join(","), selectedTrackerStatuses().join(",")].join("|")
    }
    function prepare(mode) {
        if (projectId === "" || (controller && controller.busy)) return
        pendingPreparationKey = reportKey(mode)
        if (workflow === "status-report")
            DesktopAppController.prepareStatusReportReviewWithOptions(projectId, reportDate.text, internalReport, mode, false, false)
        else if (workflow === "tracker-items-report")
            DesktopAppController.prepareTrackerReportReviewWithOptions(projectId, reportDate.text, internalReport, mode,
                                                                        selectedTrackerTypes(), selectedTrackerStatuses(), false, false)
        else
            DesktopAppController.prepareMeetingNotesReportReviewWithOptions(projectId, reportDate.text, internalReport, mode, false, false)
    }
    function prepareEmailReport() {
        if (delivery === "email" && projectId !== "" && reportDate.text !== "")
            prepare(emailMode)
    }
    function defaultPeopleSource() { return "project-team" }
    function refreshCompanies() {
        companyChoices = DesktopAppController.reviewAudienceCompanies()
        selectedCompanyIds = []
    }
    function setCompanySelected(companyId, selected) {
        var ids = selectedCompanyIds.slice()
        var index = ids.indexOf(companyId)
        if (selected && index < 0) ids.push(companyId)
        else if (!selected && index >= 0) ids.splice(index, 1)
        selectedCompanyIds = ids
    }
    function applyDefaultAudience() {
        if (projectReport)
            DesktopAppController.restoreProjectReportDefaultAudience()
        else
            DesktopAppController.applyReviewAudienceRule(defaultPeopleSource(), "all", false, false)
    }
    function applySpecificCompanies() {
        DesktopAppController.applyReviewAudienceRuleWithCompanies(defaultPeopleSource(), "selected-companies",
                                                                   selectedCompanyIds, false, false)
    }
    function defaultReportFileStem() {
        var date = (reportDate.text || "").split("/")
        var datePart = date.length === 3 ? date[2] + date[0] + date[1] : "Report"
        var row = DesktopAppController.projectRowForId(projectId)
        var project = row >= 0 ? DesktopAppController.getProjectData(row) : ({})
        var number = (project.project_number || "").toString().trim()
        return [datePart, number, workflowLabel(workflow)].filter(function(part) { return part !== "" }).join(" ")
    }
    function generatedExportName(format) {
        if (!controller) return ""
        var suffix = "." + format
        var names = controller.generatedAttachmentNames
        for (var i = 0; i < names.length; ++i)
            if (names[i].toLowerCase().endsWith(suffix)) return names[i]
        return ""
    }
    function openSaveDialog() {
        saveReportDialog.currentFile = StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/" + defaultReportFileStem()
        saveReportDialog.open()
    }
    function savePendingReport() {
        var mode = pendingSaveFormat === "html" ? "html-attachment" : "pdf-attachment"
        if (pendingSaveUrl === "" || !reviewing || preparedKey !== reportKey(mode)) return
        var name = generatedExportName(pendingSaveFormat)
        if (name === "") return
        var destination = pendingSaveUrl
        pendingSaveUrl = ""
        pendingSaveFormat = ""
        if (DesktopAppController.saveReviewGeneratedAttachment(name, destination) && openAfterSave)
            Qt.openUrlExternally(destination)
    }
    function deliverySummary() {
        var report = workflowLabel(workflow)
        if (delivery === "save") return qsTr("Will prepare %1 for %2 and ask where to save it.").arg(report).arg(reportDate.text)
        var count = recipientModel ? recipientModel.selectedRecipientCount : 0
        if (!prepared) return qsTr("Preparing %1 and its recipient options…").arg(report)
        return qsTr("Create a Microsoft 365 draft of %1 for %2 selected recipient(s), as %3.").arg(report).arg(count).arg(modeLabel(emailMode))
    }

    onWorkflowChanged: {
        preparedKey = ""
        pendingPreparationKey = ""
        selectedCompanyIds = []
        Qt.callLater(dialog.prepareEmailReport)
    }
    onDeliveryChanged: Qt.callLater(dialog.prepareEmailReport)
    onEmailModeChanged: Qt.callLater(dialog.prepareEmailReport)
    onInternalReportChanged: Qt.callLater(dialog.prepareEmailReport)
    onOpened: {
        if (reportDate.text === "") reportDate.text = todayText()
        Qt.callLater(dialog.prepareEmailReport)
    }
    onClosed: { pendingSaveUrl = ""; pendingSaveFormat = ""; pendingPreparationKey = "" }
    Connections {
        target: dialog.controller
        function onStateChanged() {
            if (dialog.controller && dialog.controller.stageName === "reviewing" && dialog.pendingPreparationKey !== "") {
                dialog.preparedKey = dialog.pendingPreparationKey
                dialog.pendingPreparationKey = ""
                dialog.refreshCompanies()
            }
            dialog.savePendingReport()
        }
    }

    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    header: null
    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 14; spacing: 8
            MaterialIcon { name: "description"; size: 20; color: Theme.accent }
            Label { text: dialog.title; color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold; Layout.fillWidth: true }
            ToolButton { text: "×"; onClicked: dialog.close() }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        ScrollView {
            id: scroll
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; padding: 16
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ColumnLayout {
                width: scroll.availableWidth; spacing: 14
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Choose a report, then save it or create a Microsoft 365 draft. You stay in control of recipients and format.")
                    color: Theme.text2; wrapMode: Text.Wrap
                }
                Rectangle {
                    Layout.fillWidth: true; radius: Theme.radius; color: Theme.surface; border.color: Theme.border
                    implicitHeight: chooseReport.implicitHeight + 28
                    ColumnLayout {
                        id: chooseReport
                        anchors.fill: parent; anchors.margins: 14; spacing: 9
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle { Layout.preferredWidth: 24; Layout.preferredHeight: 24; radius: 12; color: Theme.accent
                                Label { anchors.centerIn: parent; text: "1"; color: "white"; font.weight: Font.Bold } }
                            Label { text: qsTr("Which report?"); color: Theme.text; font.weight: Font.DemiBold; font.pixelSize: Theme.fontLg }
                        }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            ComboField {
                                Layout.fillWidth: true; label: qsTr("Report")
                                options: dialog.workflowOptions.map(function(option) { return option.value })
                                value: dialog.workflowOptionValue(dialog.workflow)
                                onActivated: function(value) { dialog.workflow = dialog.workflowFromOption(value) }
                            }
                            DateField {
                                id: reportDate; Layout.preferredWidth: 158; label: qsTr("Reporting date")
                                onEdited: dialog.prepareEmailReport()
                            }
                        }
                        CompactCheckBox {
                            text: dialog.workflow === "meeting-notes-report" ? qsTr("Include internal meetings") : qsTr("Generate internal report")
                            checked: dialog.internalReport
                            onToggled: dialog.internalReport = checked
                        }
                        ColumnLayout {
                            visible: dialog.workflow === "tracker-items-report"; Layout.fillWidth: true; spacing: 3
                            Label { text: qsTr("Include tracker items"); color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
                            RowLayout {
                                Layout.fillWidth: true
                                CompactCheckBox { id: includeTrackerItems; text: qsTr("Tracker"); checked: true; onToggled: dialog.prepareEmailReport() }
                                CompactCheckBox { id: includeActionItems; text: qsTr("Action"); onToggled: dialog.prepareEmailReport() }
                            }
                            Label { text: qsTr("Status"); color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
                            Flow {
                                Layout.fillWidth: true; spacing: 9
                                CompactCheckBox { id: includeNew; text: qsTr("New"); checked: true; onToggled: dialog.prepareEmailReport() }
                                CompactCheckBox { id: includeAssigned; text: qsTr("Assigned"); checked: true; onToggled: dialog.prepareEmailReport() }
                                CompactCheckBox { id: includeResolved; text: qsTr("Resolved"); onToggled: dialog.prepareEmailReport() }
                                CompactCheckBox { id: includeDeferred; text: qsTr("Deferred"); onToggled: dialog.prepareEmailReport() }
                                CompactCheckBox { id: includeCancelled; text: qsTr("Cancelled"); onToggled: dialog.prepareEmailReport() }
                            }
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; radius: Theme.radius; color: Theme.surface; border.color: Theme.border
                    implicitHeight: deliveryOptions.implicitHeight + 28
                    ColumnLayout {
                        id: deliveryOptions
                        anchors.fill: parent; anchors.margins: 14; spacing: 9
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle { Layout.preferredWidth: 24; Layout.preferredHeight: 24; radius: 12; color: Theme.accent
                                Label { anchors.centerIn: parent; text: "2"; color: "white"; font.weight: Font.Bold } }
                            Label { text: qsTr("What should happen with it?"); color: Theme.text; font.weight: Font.DemiBold; font.pixelSize: Theme.fontLg }
                        }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            ChoiceCard {
                                Layout.fillWidth: true; chosen: dialog.delivery === "save"; iconName: "save"
                                heading: qsTr("Save the report"); detail: qsTr("Choose a location. Nothing is sent.")
                                onSelected: dialog.delivery = "save"
                            }
                            ChoiceCard {
                                Layout.fillWidth: true; chosen: dialog.delivery === "email"; iconName: "mail"
                                heading: qsTr("Email the report"); detail: qsTr("Create a draft for selected recipients.")
                                onSelected: dialog.delivery = "email"
                            }
                        }
                    }
                }
                Rectangle {
                    visible: dialog.delivery === "email"; Layout.fillWidth: true; radius: Theme.radius; color: Theme.surface; border.color: Theme.border
                    implicitHeight: emailOptions.implicitHeight + 28
                    ColumnLayout {
                        id: emailOptions
                        anchors.fill: parent; anchors.margins: 14; spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle { Layout.preferredWidth: 24; Layout.preferredHeight: 24; radius: 12; color: Theme.accent
                                Label { anchors.centerIn: parent; text: "3"; color: "white"; font.weight: Font.Bold } }
                            Label { text: qsTr("Email details"); color: Theme.text; font.weight: Font.DemiBold; font.pixelSize: Theme.fontLg }
                        }
                        Label { text: qsTr("Format"); color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
                        Repeater {
                            model: dialog.emailModeOptions
                            delegate: ChoiceCard {
                                required property var modelData
                                chosen: dialog.emailMode === modelData.value
                                iconName: modelData.value === "pdf-attachment" ? "picture_as_pdf" : "code"
                                heading: modelData.label; detail: modelData.description
                                onSelected: dialog.emailMode = modelData.value
                            }
                        }
                        BusyIndicator { running: dialog.controller && dialog.controller.busy; visible: running; Layout.alignment: Qt.AlignHCenter }
                        Label {
                            visible: dialog.controller && dialog.controller.diagnostic !== ""
                            Layout.fillWidth: true; text: dialog.controller ? dialog.controller.diagnostic : ""
                            color: Theme.red; wrapMode: Text.Wrap
                        }
                        ColumnLayout {
                            visible: dialog.delivery === "email"; Layout.fillWidth: true; spacing: 9
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
                            Label { text: qsTr("Send to"); color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
                            RowLayout {
                                Layout.fillWidth: true; spacing: 8
                                ChoiceCard {
                                    Layout.fillWidth: true; implicitHeight: 58; chosen: !specificCompanies.checked
                                    iconName: "groups"; heading: qsTr("Default audience"); detail: qsTr("Use this report’s normal distribution.")
                                    onSelected: { specificCompanies.checked = false; dialog.applyDefaultAudience() }
                                }
                                ChoiceCard {
                                    Layout.fillWidth: true; implicitHeight: 58; chosen: specificCompanies.checked
                                    iconName: "business"; heading: qsTr("Specific companies"); detail: qsTr("Limit the default audience by company.")
                                    onSelected: specificCompanies.checked = true
                                }
                            }
                            CheckBox { id: specificCompanies; visible: false }
                            Flow {
                                visible: specificCompanies.checked; Layout.fillWidth: true; spacing: 7
                                Repeater {
                                    model: dialog.companyChoices
                                    delegate: CompactCheckBox {
                                        required property var modelData
                                        text: qsTr("%1 (%2)").arg(modelData.name).arg(modelData.peopleCount)
                                        checked: dialog.selectedCompanyIds.indexOf(modelData.id) >= 0
                                        onToggled: { dialog.setCompanySelected(modelData.id, checked); dialog.applySpecificCompanies() }
                                    }
                                }
                            }
                            Label {
                                visible: !dialog.prepared
                                Layout.fillWidth: true
                                text: qsTr("Loading report recipients…")
                                color: Theme.text3
                            }
                            RecipientSelectionPane {
                                visible: dialog.prepared
                                Layout.fillWidth: true
                                recipientModel: dialog.recipientModel
                                reviewController: dialog.controller
                                audienceController: dialog.projectReport ? null : DesktopAppController
                                defaultPeopleSource: dialog.defaultPeopleSource()
                                compact: true
                                fixedAudience: dialog.projectReport
                            }
                            Label {
                                Layout.fillWidth: true
                                text: dialog.graphDraftReady
                                    ? qsTr("A draft will be created in the verified Microsoft 365 account%1. It will not be sent.")
                                        .arg(dialog.office365.accountLabel === "" ? "" : " (" + dialog.office365.accountLabel + ")")
                                    : qsTr("Your configured email client will open with this report and the selected recipients.")
                                color: Theme.text3; wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 14; spacing: 10
            Label { Layout.fillWidth: true; text: dialog.deliverySummary(); color: Theme.text2; wrapMode: Text.Wrap }
            Button { text: qsTr("Cancel"); onClicked: dialog.close() }
            Button {
                visible: dialog.delivery === "save"; text: qsTr("Save As…")
                enabled: dialog.projectId !== "" && (!dialog.controller || !dialog.controller.busy)
                onClicked: dialog.openSaveDialog()
            }
            Button {
                visible: dialog.delivery === "email"
                text: dialog.graphDraftReady ? qsTr("Create Microsoft 365 draft") : qsTr("Send email")
                enabled: dialog.prepared && dialog.recipientModel
                         && dialog.recipientModel.selectedRecipientCount > 0 && !dialog.controller.busy
                onClicked: DesktopAppController.handoffPreparedReview()
            }
            Button {
                visible: dialog.controller && dialog.controller.stageName === "completed" && dialog.controller.presentationUrl.toString() !== ""
                text: qsTr("Open draft in Outlook")
                onClicked: Qt.openUrlExternally(dialog.controller.presentationUrl)
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
            if (!destination.toLowerCase().endsWith("." + dialog.pendingSaveFormat)) destination += "." + dialog.pendingSaveFormat
            if (!DesktopAppController.removeExistingReportSaveFile(destination)) return
            dialog.pendingSaveUrl = destination
            dialog.prepare(dialog.pendingSaveFormat === "html" ? "html-attachment" : "pdf-attachment")
        }
    }
}

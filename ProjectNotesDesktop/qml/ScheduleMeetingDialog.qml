// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Schedule Meeting review.  The meeting type, project snapshot, saved
// audiences and calendar handoff all stay native; this dialog only picks the
// meeting type and the attendees, with the same pane the reports use.
Dialog {
    id: dialog
    objectName: "scheduleMeetingDialog"
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(720, parent ? parent.width - 36 : 720)
    height: Math.min(820, parent ? parent.height - 36 : 820)
    padding: 0
    title: qsTr("Schedule meeting")

    // Clicking away dismisses the dialog and nothing else — see ClickShield.qml.
    ClickShield { host: dialog }

    // Close on the next tick rather than inside a click — see
    // FilterDialog._dismiss().
    function _dismiss() { Qt.callLater(close) }

    property string projectId: ""
    property var recipientModel: DesktopAppController.recipientSelectionModel
    property var meetingTypes: []
    property string templateId: ""
    property var review: ({ ready: false, loading: false })
    property string errorText: ""
    // Confirmation after "Copy Attendees", shown in place of the backend
    // description for a few seconds.
    property string copyNotice: ""

    readonly property bool busy: DesktopAppController.meetingHandoffBusy
    readonly property bool ready: review.ready === true
    readonly property var office365: DesktopAppController.office365SettingsModel

    function templateName(id) {
        for (var i = 0; i < meetingTypes.length; ++i)
            if (meetingTypes[i].id === id) return meetingTypes[i].name
        return ""
    }
    function templateIdForName(name) {
        for (var i = 0; i < meetingTypes.length; ++i)
            if (meetingTypes[i].name === name) return meetingTypes[i].id
        return ""
    }
    function prepare() {
        errorText = ""
        if (projectId === "" || templateId === "") return
        DesktopAppController.prepareMeetingReview(projectId, templateId)
    }
    function loadReview() {
        review = DesktopAppController.meetingReview()
    }
    function backendDescription() {
        switch (review.backend) {
        case "graph":
            return qsTr("A draft meeting will be created in your Microsoft 365 calendar%1 and opened in Outlook. Edit the time, subject and agenda there, then send the invitations.")
                .arg(office365 && office365.accountLabel !== "" ? " (" + office365.accountLabel + ")" : "")
        case "mac-mail":
            return qsTr("The meeting will be created in Calendar without attendees. Copy the attendees, paste them into the event's invitees, then click Send.")
        case "outlook":
            return qsTr("The meeting will open in the new Outlook without attendees. Copy the attendees, paste them into the event, then send the invitations.")
        case "outlook-classic":
            return qsTr("The meeting will open in Outlook (classic) with the attendees filled in. Review it there, then click Send.")
        case "thunderbird":
            return qsTr("Thunderbird will offer to import the meeting into one of its calendars. Open the event there to edit it and send the invitations.")
        default:
            return qsTr("A calendar file (.ics) will open in your default calendar app. Some calendar apps leave out the attendees, so copy them first if you need to. Edit and save the event there, then send the invitations.")
        }
    }
    function schedule() {
        errorText = ""
        // Everything but the attendees comes from the meeting type; the user
        // edits it in the calendar app after the invite opens.
        DesktopAppController.handoffPreparedMeeting(review.subject, review.agendaHtml, review.location,
                                                    review.date, review.time, review.durationMinutes)
    }

    function copyAttendees() {
        var count = DesktopAppController.copyMeetingAttendees()
        copyNotice = count === 0 ? qsTr("No attendees are selected.")
                   : count === 1 ? qsTr("Copied 1 attendee to the clipboard.")
                   : qsTr("Copied %1 attendees to the clipboard.").arg(count)
        copyNoticeTimer.restart()
    }

    Timer { id: copyNoticeTimer; interval: 4000; onTriggered: dialog.copyNotice = "" }

    onOpened: {
        errorText = ""
        copyNotice = ""
        review = ({ ready: false, loading: false })
        meetingTypes = DesktopAppController.meetingTemplateChoices()
        if (templateName(templateId) === "")
            templateId = meetingTypes.length > 0 ? meetingTypes[0].id : ""
        Qt.callLater(dialog.prepare)
    }
    onClosed: DesktopAppController.closeMeetingReview()

    Connections {
        target: DesktopAppController
        function onMeetingReviewChanged() {
            if (!dialog.visible) return
            // Keep the user's edits while a handoff runs or reports a failure.
            var next = DesktopAppController.meetingReview()
            if (next.ready && dialog.ready && next.templateName === dialog.review.templateName) {
                dialog.review = next
                return
            }
            dialog.loadReview()
        }
        function onMeetingHandoffFinished(succeeded, message) {
            if (!dialog.visible) return
            if (succeeded) dialog._dismiss()
            else dialog.errorText = message
        }
    }

    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    header: null
    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 14; spacing: 8
            MaterialIcon { name: "event"; size: 20; color: Theme.accent }
            Label { text: dialog.title; color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold; Layout.fillWidth: true }
            ToolButton { text: "×"; onClicked: dialog._dismiss() }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        ScrollView {
            id: scroll
            enabled: !dialog.busy
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; padding: 16
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ColumnLayout {
                width: scroll.availableWidth; spacing: 14

                Rectangle {
                    Layout.fillWidth: true; radius: Theme.radius; color: Theme.surface; border.color: Theme.border
                    implicitHeight: chooseType.implicitHeight + 28
                    ColumnLayout {
                        id: chooseType
                        anchors.fill: parent; anchors.margins: 14; spacing: 9
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle { Layout.preferredWidth: 24; Layout.preferredHeight: 24; radius: 12; color: Theme.accent
                                Label { anchors.centerIn: parent; text: "1"; color: "white"; font.weight: Font.Bold } }
                            Label { text: qsTr("Which meeting?"); color: Theme.text; font.weight: Font.DemiBold; font.pixelSize: Theme.fontLg }
                        }
                        ComboField {
                            visible: dialog.meetingTypes.length > 0
                            Layout.fillWidth: true; label: qsTr("Meeting type")
                            options: dialog.meetingTypes.map(function(type) { return type.name })
                            value: dialog.templateName(dialog.templateId)
                            onActivated: function(value) {
                                dialog.templateId = dialog.templateIdForName(value)
                                dialog.prepare()
                            }
                        }
                        Label {
                            visible: dialog.meetingTypes.length === 0
                            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.amber
                            text: qsTr("There are no meeting types yet. Add one in Settings > Meeting Templates, then schedule the meeting again.")
                        }
                        Label {
                            visible: dialog.meetingTypes.length > 0 && !dialog.ready
                            Layout.fillWidth: true; color: Theme.text3
                            text: qsTr("Preparing the meeting…")
                        }
                    }
                }

                Rectangle {
                    visible: dialog.ready
                    Layout.fillWidth: true; radius: Theme.radius; color: Theme.surface; border.color: Theme.border
                    implicitHeight: attendees.implicitHeight + 28
                    ColumnLayout {
                        id: attendees
                        anchors.fill: parent; anchors.margins: 14; spacing: 9
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle { Layout.preferredWidth: 24; Layout.preferredHeight: 24; radius: 12; color: Theme.accent
                                Label { anchors.centerIn: parent; text: "2"; color: "white"; font.weight: Font.Bold } }
                            Label { text: qsTr("Who should attend?"); color: Theme.text; font.weight: Font.DemiBold; font.pixelSize: Theme.fontLg }
                        }
                        RecipientSelectionPane {
                            Layout.fillWidth: true
                            recipientModel: dialog.recipientModel
                            audienceController: DesktopAppController
                            meetingMode: true
                            compact: true
                        }
                    }
                }
            }
        }
        ProgressBar {
            Layout.fillWidth: true
            visible: dialog.busy || (dialog.meetingTypes.length > 0 && !dialog.ready)
            indeterminate: true
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        ColumnLayout {
            Layout.fillWidth: true; Layout.margins: 14; spacing: 8
            Label {
                visible: dialog.errorText !== ""
                Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.red
                text: dialog.errorText
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 10
                Label {
                    Layout.fillWidth: true; color: Theme.text2; wrapMode: Text.Wrap
                    text: dialog.busy ? qsTr("Scheduling the meeting…")
                        : dialog.copyNotice !== "" ? dialog.copyNotice
                        : dialog.ready ? dialog.backendDescription() : ""
                }
                // Calendar can't be handed attendees without sending the
                // invitations at once, and the new Outlook (and other apps
                // opening the .ics) drop them, so they travel by clipboard.
                // Only Outlook (classic) and Microsoft 365 fill them in.
                Button {
                    visible: ["default-app", "outlook", "mac-mail"].indexOf(dialog.review.backend) >= 0
                    text: qsTr("Copy Attendees")
                    enabled: dialog.ready && !dialog.busy
                    onClicked: dialog.copyAttendees()
                }
                Button { text: qsTr("Cancel"); onClicked: dialog._dismiss() }
                Button {
                    primary: true
                    text: qsTr("Open Invite")
                    enabled: dialog.ready && !dialog.busy
                    onClicked: dialog.schedule()
                }
            }
        }
    }
}

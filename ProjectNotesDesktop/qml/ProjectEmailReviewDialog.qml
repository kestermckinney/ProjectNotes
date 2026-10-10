// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

Dialog {
    id: dialog
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(720, parent ? parent.width - 36 : 720)
    height: Math.min(760, parent ? parent.height - 36 : 760)
    title: sharingLocation ? qsTr("Share Location") : plainContact ? qsTr("Email") : qsTr("Project Email")
    objectName: "projectEmailReviewDialog"
    padding: 16
    property bool sharingLocation: false
    property bool plainContact: false
    readonly property var controller: DesktopAppController.communicationsController
    readonly property var recipients: DesktopAppController.recipientSelectionModel
    readonly property bool ready: controller && controller.stageName === "reviewing" && !controller.busy
    ClickShield { host: dialog }
    Connections {
        target: dialog.controller
        function onStateChanged() {
            if (dialog.visible && dialog.controller.stageName === "completed") Qt.callLater(dialog.close)
        }
    }
    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    header: Item {
        implicitHeight: 50
        RowLayout {
            anchors.fill: parent; anchors.margins: 14
            MaterialIcon { name: "email"; size: 20; color: Theme.accent }
            Label { text: dialog.title; color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold; Layout.fillWidth: true }
            ToolButton { text: "×"; enabled: !dialog.controller || !dialog.controller.busy; onClicked: Qt.callLater(dialog.close) }
        }
    }
    onClosed: {
        if (!controller || !controller.busy)
            DesktopAppController.cancelProjectEmailReview()
    }
    function openProject(projectId, contactId) {
        sharingLocation = false
        plainContact = false
        if (DesktopAppController.prepareProjectEmailReview(projectId, contactId)) open()
    }
    function openContact(contactId) {
        sharingLocation = false
        plainContact = true
        if (DesktopAppController.prepareContactEmailReview(contactId)) open()
    }
    function openLocation(projectId, locationId, attachment) {
        sharingLocation = true
        plainContact = false
        if (DesktopAppController.prepareLocationEmailReview(projectId, locationId, attachment)) open()
    }
    contentItem: ScrollView {
        id: scroll
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 12
            BusyIndicator { running: dialog.controller && (dialog.controller.stageName === "editing" || dialog.controller.busy); visible: running; Layout.alignment: Qt.AlignHCenter }
            Label {
                Layout.fillWidth: true
                visible: dialog.controller && dialog.controller.diagnostic !== ""
                text: dialog.controller ? dialog.controller.diagnostic : ""
                color: Theme.red; wrapMode: Text.Wrap
            }
            Label {
                Layout.fillWidth: true
                text: dialog.plainContact
                    ? qsTr("Create a blank draft for this contact, then write your message in your email client.")
                    : qsTr("Review the recipients, then create a draft using your configured email template. Review and edit the message in your email client.")
                color: Theme.text2; wrapMode: Text.Wrap
            }
            Label {
                Layout.fillWidth: true; wrapMode: Text.Wrap
                visible: DesktopAppController.preferredEmailBackend === "mac-mail" && dialog.recipients && dialog.recipients.bccRecipientCount > 0
                text: qsTr("MacOS Email cannot use Bcc. Choose another integration or change these recipients to To or Cc.")
                color: Theme.amber
            }
            Label {
                visible: dialog.plainContact && dialog.ready
                text: dialog.recipients ? qsTr("To: %1").arg(dialog.recipients.selectedRecipientCount > 0
                    ? dialog.controller.recipientLabel : qsTr("The selected contact has no valid email address.")) : ""
                color: Theme.text2
                Layout.fillWidth: true; wrapMode: Text.Wrap
            }
            RecipientSelectionPane {
                objectName: "projectEmailAudience"
                Layout.fillWidth: true
                visible: !dialog.plainContact
                enabled: dialog.ready
                recipientModel: dialog.recipients
                reviewController: dialog.controller
                audienceController: DesktopAppController
                compact: true
                fixedAudience: false
                defaultAudienceLabel: dialog.sharingLocation ? qsTr("sharing") : qsTr("project email")
            }
            Label {
                Layout.fillWidth: true; wrapMode: Text.Wrap
                visible: dialog.controller && dialog.controller.attachmentNames.length > 0
                text: dialog.controller ? qsTr("Attachments: %1").arg(dialog.controller.attachmentNames.join(", ")) : ""
                color: Theme.text2
            }
        }
    }
    footer: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: dialog.leftPadding
            anchors.rightMargin: dialog.rightPadding
            spacing: 10
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Cancel"); enabled: !dialog.controller || !dialog.controller.busy; onClicked: Qt.callLater(dialog.close) }
            Button {
                objectName: "projectEmailDraftButton"
                text: qsTr("Create Email Draft")
                enabled: dialog.ready && dialog.recipients && dialog.recipients.selectedRecipientCount > 0
                         && !(DesktopAppController.preferredEmailBackend === "mac-mail" && dialog.recipients.bccRecipientCount > 0)
                onClicked: DesktopAppController.handoffPreparedReview()
            }
        }
    }
}

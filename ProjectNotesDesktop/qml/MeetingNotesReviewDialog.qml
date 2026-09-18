// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Review-only surface for the native Send Notes pipeline. The controller owns
// snapshotting, content generation, recipient resolution, and backend handoff.
Dialog {
    id: dialog
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(720, parent ? parent.width - 36 : 720)
    height: Math.min(760, parent ? parent.height - 36 : 760)
    padding: 0
    title: qsTr("Send Notes")

    property var controller: null
    property var recipientModel: null
    readonly property var office365: DesktopAppController.office365SettingsModel
    readonly property bool graphDraftReady: DesktopAppController.preferredEmailBackend === "graph"
    readonly property bool reviewing: controller && controller.stageName === "reviewing" && !controller.busy

    component SectionCard: Rectangle {
        property string step: ""
        property string heading: ""
        default property alias contents: cardContents.data
        Layout.fillWidth: true
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
        implicitHeight: cardContents.implicitHeight + 28
        ColumnLayout {
            id: cardContents
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Rectangle {
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24; radius: 12
                    color: Theme.accent
                    Label { anchors.centerIn: parent; text: step; color: "white"; font.weight: Font.Bold }
                }
                Label { text: heading; color: Theme.text; font.weight: Font.DemiBold; font.pixelSize: Theme.fontLg }
            }
        }
    }

    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    header: null
    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 14; spacing: 8
            MaterialIcon { name: "email"; size: 20; color: Theme.accent }
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
                width: scroll.availableWidth
                spacing: 14
                Label {
                    Layout.fillWidth: true
                    text: dialog.reviewing
                        ? qsTr("Attendees are selected as To recipients. Other team members are unchecked and default to Cc. The project manager is excluded.")
                        : qsTr("Preparing the current saved meeting note…")
                    color: Theme.text2; wrapMode: Text.Wrap
                }
                BusyIndicator { running: dialog.controller && dialog.controller.stageName === "editing"; visible: running; Layout.alignment: Qt.AlignHCenter }
                Label {
                    Layout.fillWidth: true; visible: dialog.controller && dialog.controller.diagnostic !== ""
                    text: dialog.controller ? dialog.controller.diagnostic : ""; color: Theme.red; wrapMode: Text.Wrap
                }
                SectionCard {
                    step: "1"; heading: qsTr("Send to")
                    RecipientSelectionPane {
                        Layout.fillWidth: true; recipientModel: dialog.recipientModel; reviewController: dialog.controller
                        compact: true; fixedAudience: true; recipientListOnly: true
                    }
                    Label {
                        Layout.fillWidth: true
                        text: dialog.graphDraftReady ? qsTr("A draft will be created in Microsoft 365%1. It will not be sent.")
                            .arg(dialog.office365 && dialog.office365.accountLabel !== "" ? " (" + dialog.office365.accountLabel + ")" : "")
                            : qsTr("Your configured email client will open with the meeting notes and selected recipients.")
                        color: Theme.text3; wrapMode: Text.Wrap
                    }
                }
            }
        }
        ProgressBar {
            Layout.fillWidth: true
            visible: dialog.controller && (dialog.controller.busy || dialog.controller.stageName === "editing")
            indeterminate: true
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 14; spacing: 10
            Label {
                Layout.fillWidth: true; color: Theme.text2; wrapMode: Text.Wrap
                text: dialog.controller && dialog.controller.stageName === "completed" && dialog.controller.draftIdentity !== ""
                    ? qsTr("Microsoft 365 draft created.") : ""
            }
            Button { text: qsTr("Cancel"); onClicked: dialog.close() }
            Button {
                text: qsTr("Send Email")
                enabled: dialog.reviewing && dialog.recipientModel && dialog.recipientModel.selectedRecipientCount > 0
                onClicked: DesktopAppController.handoffPreparedReview()
            }
        }
    }
}

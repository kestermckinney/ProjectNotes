// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ProjectNotesDesktop

// Review-only surface for the native Send Meeting Notes pipeline. The desktop
// controller performs all snapshot/report/audience work; this dialog never
// calculates content or activates a mail client.
Dialog {
    id: dialog
    modal: true
    anchors.centerIn: parent
    width: Math.min(620, parent ? parent.width - 36 : 620)
    height: Math.min(620, parent ? parent.height - 36 : 620)
    padding: 0
    title: qsTr("Review Meeting Notes Email")
    property var controller: null
    property var recipientModel: null
    property var templateModel: null
    readonly property var office365: DesktopAppController.office365SettingsModel
    readonly property bool graphDraftReady: DesktopAppController.preferredEmailBackend === "graph"
                                         && office365 && office365.emailDraftsGranted
                                         && office365.accountLabel !== ""
    onOpened: if (templateModel) templateModel.workflow = "send-meeting-notes"

    background: Rectangle {
        radius: Theme.radius
        color: Theme.raise
        border.color: Theme.border
    }

    header: RowLayout {
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

    contentItem: ColumnLayout {
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: dialog.controller && dialog.controller.stageName === "reviewing"
                ? dialog.graphDraftReady
                    ? qsTr("Review recipients and create a Microsoft 365 draft when ready.")
                    : qsTr("Review recipients before email delivery is configured.")
                : qsTr("Preparing the current saved meeting note…")
            color: Theme.text2
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.controller && dialog.controller.diagnostic !== ""
            text: dialog.controller ? dialog.controller.diagnostic : ""
            color: Theme.amber
            wrapMode: Text.Wrap
        }
        ColumnLayout {
            Layout.fillWidth: true
            visible: dialog.controller && dialog.controller.stageName === "reviewing"
            spacing: 4
            Label { text: qsTr("Subject"); color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
        TextField {
                id: subjectField
                Layout.fillWidth: true
                text: dialog.controller ? dialog.controller.subject : ""
                color: Theme.text
                enabled: dialog.controller && dialog.controller.stageName === "reviewing"
                onEditingFinished: if (dialog.controller) dialog.controller.setSubject(text)
            }
            Label {
                visible: dialog.controller && dialog.controller.inlineHtmlMode
                text: qsTr("Message HTML")
                color: Theme.text3
                font.pixelSize: Theme.fontXs
                font.weight: Font.DemiBold
            }
            TextArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 112
                visible: dialog.controller && dialog.controller.inlineHtmlMode
                text: dialog.controller ? dialog.controller.htmlBody : ""
                color: Theme.text
                wrapMode: TextArea.Wrap
                onActiveFocusChanged: if (!activeFocus && dialog.controller) dialog.controller.setHtmlBody(text)
            }
            Label {
                visible: !dialog.controller || !dialog.controller.inlineHtmlMode
                text: qsTr("Message preview")
                color: Theme.text3
                font.pixelSize: Theme.fontXs
                font.weight: Font.DemiBold
            }
            Label {
                Layout.fillWidth: true
                Layout.maximumHeight: 96
                visible: !dialog.controller || !dialog.controller.inlineHtmlMode
                text: dialog.controller ? dialog.controller.plainText : ""
                color: Theme.text2
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                maximumLineCount: 5
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Template"); color: Theme.text2 }
            ComboBox {
                id: meetingTemplate
                Layout.fillWidth: true
                textRole: "name"
                valueRole: "id"
                model: {
                    const defaults = [{ id: "", name: qsTr("Native default") }]
                    return defaults.concat(dialog.templateModel ? dialog.templateModel.templates : [])
                }
                enabled: dialog.controller && dialog.controller.stageName === "reviewing" && !dialog.controller.busy
                onActivated: {
                    if (dialog.templateModel && currentValue !== "")
                        dialog.templateModel.selectTemplate(currentValue)
                    DesktopAppController.applyMeetingNotesTemplate(currentValue)
                }
            }
        }
        BusyIndicator {
            running: dialog.controller && dialog.controller.stageName === "editing"
            visible: running
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.controller && dialog.controller.diagnostic.length > 0
            text: dialog.controller ? dialog.controller.diagnostic : ""
            color: Theme.red
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.controller && dialog.controller.attachmentNames.length > 0
            text: qsTr("Attachments: %1").arg(dialog.controller ? dialog.controller.attachmentNames.join(", ") : "")
            color: Theme.text2
            wrapMode: Text.Wrap
        }
        Button {
            text: qsTr("Add attachment")
            enabled: dialog.controller && dialog.controller.stageName === "reviewing" && !dialog.controller.busy
            onClicked: userAttachmentDialog.open()
        }
        Button {
            visible: dialog.controller && dialog.controller.generatedAttachmentNames.length > 0
            text: qsTr("Save generated attachment")
            enabled: dialog.controller && dialog.controller.stageName === "reviewing" && !dialog.controller.busy
            onClicked: saveGeneratedAttachmentDialog.open()
        }
        Button {
            visible: dialog.controller && dialog.controller.plainText.trim().length > 0
            text: qsTr("Copy plain-text message")
            enabled: dialog.controller && dialog.controller.stageName === "reviewing" && !dialog.controller.busy
            onClicked: DesktopAppController.copyReviewPlainText()
        }
        RecipientSelectionPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            recipientModel: dialog.recipientModel
            reviewController: dialog.controller
            audienceController: DesktopAppController
            defaultExcludeProjectManager: true
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        Label {
            Layout.fillWidth: true
            text: dialog.graphDraftReady
                ? qsTr("A draft will be created in the verified Microsoft 365 account%1. It will not be sent.")
                    .arg(dialog.office365.accountLabel === "" ? "" : " (" + dialog.office365.accountLabel + ")")
                : qsTr("Email delivery is not configured in this preview.")
            color: Theme.text3
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.controller && dialog.controller.stageName === "completed"
                     && dialog.controller.draftIdentity !== ""
            text: qsTr("Microsoft 365 draft created.")
            color: Theme.green
            wrapMode: Text.Wrap
        }
        Button {
            visible: dialog.controller && dialog.controller.stageName === "completed"
                     && dialog.controller.presentationUrl.toString() !== ""
            text: qsTr("Open draft in Outlook")
            onClicked: Qt.openUrlExternally(dialog.controller.presentationUrl)
        }
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                visible: dialog.graphDraftReady
                text: qsTr("Create Microsoft 365 draft")
                enabled: dialog.controller && dialog.controller.stageName === "reviewing" && !dialog.controller.busy
                onClicked: DesktopAppController.handoffPreparedReview()
            }
            Button { text: qsTr("Close"); onClicked: dialog.close() }
        }
    }
    FileDialog {
        id: userAttachmentDialog
        fileMode: FileDialog.OpenFile
        onAccepted: DesktopAppController.addReviewAttachment(selectedFile.toString())
    }
    FileDialog {
        id: saveGeneratedAttachmentDialog
        fileMode: FileDialog.SaveFile
        onAccepted: if (dialog.controller && dialog.controller.generatedAttachmentNames.length > 0)
                        DesktopAppController.saveReviewGeneratedAttachment(dialog.controller.generatedAttachmentNames[0], selectedFile.toString())
    }
}

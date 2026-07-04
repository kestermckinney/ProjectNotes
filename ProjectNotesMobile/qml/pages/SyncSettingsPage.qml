// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Cloud Sync Settings")

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            // ── Sync toggle ───────────────────────────────────────────────────
            SectionHeader { text: qsTr("Sync") }

            SettingsRow {
                label: qsTr("Enable Sync")
                control: Switch {
                    checked: AppController.syncEnabled
                    onToggled: AppController.syncEnabled = checked
                }
            }

            // ── Credentials ───────────────────────────────────────────────────
            SectionHeader { text: qsTr("Email") }
            FieldRow {
                FormField {
                    text: AppController.syncEmail
                    placeholderText: qsTr("user@example.com")
                    inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoPredictiveText
                    onEditingFinished: AppController.syncEmail = text
                }
            }

            SectionHeader { text: qsTr("Password") }
            FieldRow {
                FormField {
                    text: AppController.syncPassword
                    placeholderText: qsTr("password")
                    echoMode: TextInput.Password
                    onEditingFinished: AppController.syncPassword = text
                }
            }

            SectionHeader { text: qsTr("Encryption Phrase") }
            FieldRow {
                FormField {
                    text: AppController.syncEncryptionPhrase
                    placeholderText: qsTr("optional passphrase")
                    onEditingFinished: AppController.syncEncryptionPhrase = text
                }
            }

            // ── Subscription status ───────────────────────────────────────────
            SectionHeader { text: qsTr("Subscription") }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: subscriptionLabel.implicitHeight + 24
                color: palette.base

                Label {
                    id: subscriptionLabel
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: AppController.subscriptionStatusText.length > 0
                          ? AppController.subscriptionStatusText
                          : qsTr("Not connected — enable sync to view subscription status")
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                }

                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1
                    color: Theme.mutedText
                    opacity: 0.3
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: palette.base

                Label {
                    anchors.centerIn: parent
                    text: "<a href=\"https://www.projectnotespro.com\">www.projectnotespro.com</a>"
                    textFormat: Text.RichText
                    horizontalAlignment: Text.AlignHCenter
                    onLinkActivated: Qt.openUrlExternally(link)
                }

                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1
                    color: Theme.mutedText
                    opacity: 0.3
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                horizontalAlignment: Text.AlignHCenter
                text: AppController.supabaseConnectionInfo
                font.pixelSize: 11
                color: Theme.mutedText
                opacity: 0.6
            }
        }
    }

    // ── Helper components ─────────────────────────────────────────────────────

    component SectionHeader: Label {
        Layout.fillWidth: true
        Layout.topMargin: 20
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: 13
        font.weight: 600
        color: Theme.navyMid
        background: Rectangle { color: Theme.sectionBg }
    }

    component SettingsRow: RowLayout {
        property string label: ""
        property alias control: controlSlot.children

        Layout.fillWidth: true
        Layout.preferredHeight: 44
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: palette.base

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }

                Label {
                    text: parent.parent.parent.label
                    Layout.fillWidth: true
                }

                Item {
                    id: controlSlot
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    implicitWidth: children.length > 0 ? children[0].implicitWidth : 0
                    implicitHeight: children.length > 0 ? children[0].implicitHeight : 0
                }
            }

            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                height: 1
                color: Theme.mutedText
                opacity: 0.3
            }
        }
    }
}

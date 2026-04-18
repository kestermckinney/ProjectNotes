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

            // ── Remote host ───────────────────────────────────────────────────
            SectionHeader { text: qsTr("Host Type") }
            FieldRow {
                ComboBox {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: [qsTr("Self-Hosted PostgREST"), qsTr("Supabase"), qsTr("Neon")]
                    currentIndex: AppController.syncHostType
                    onActivated: AppController.syncHostType = currentIndex
                }
            }

            SectionHeader { text: qsTr("Server URL") }
            FieldRow {
                TextField {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: AppController.syncPostgrestUrl
                    placeholderText: AppController.syncHostType === 1
                        ? qsTr("https://xyz.supabase.co")
                        : AppController.syncHostType === 2
                            ? qsTr("https://your-project.neon.tech")
                            : qsTr("https://your-server/rest/v1")
                    inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText
                    background: Item {}
                    onEditingFinished: AppController.syncPostgrestUrl = text
                }
            }

            // Supabase anon key — only shown for Supabase host type
            SectionHeader {
                visible: AppController.syncHostType === 1
                text: qsTr("Supabase Key")
            }
            FieldRow {
                visible: AppController.syncHostType === 1
                TextField {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: AppController.syncSupabaseKey
                    placeholderText: qsTr("anon key")
                    background: Item {}
                    onEditingFinished: AppController.syncSupabaseKey = text
                }
            }

            // ── Credentials ───────────────────────────────────────────────────
            SectionHeader { text: qsTr("Email") }
            FieldRow {
                TextField {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: AppController.syncEmail
                    placeholderText: qsTr("user@example.com")
                    inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoPredictiveText
                    background: Item {}
                    onEditingFinished: AppController.syncEmail = text
                }
            }

            SectionHeader { text: qsTr("Password") }
            FieldRow {
                TextField {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: AppController.syncPassword
                    placeholderText: qsTr("password")
                    echoMode: TextInput.Password
                    background: Item {}
                    onEditingFinished: AppController.syncPassword = text
                }
            }

            SectionHeader { text: qsTr("Encryption Phrase") }
            FieldRow {
                TextField {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: AppController.syncEncryptionPhrase
                    placeholderText: qsTr("optional passphrase")
                    background: Item {}
                    onEditingFinished: AppController.syncEncryptionPhrase = text
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    // ── Helper components ─────────────────────────────────────────────────────

    component SectionHeader: Label {
        Layout.fillWidth: true
        Layout.topMargin: 20
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: 13
        font.weight: Font.Semibold
        color: "#0A7AFF"
        background: Rectangle { color: Qt.rgba(10/255, 122/255, 255/255, 0.06) }
    }

    component FieldRow: Rectangle {
        default property alias content: innerItem.data

        Layout.fillWidth: true
        Layout.preferredHeight: 44
        color: palette.base

        Item {
            id: innerItem
            anchors.fill: parent
        }

        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
            height: 1
            color: palette.mid
            opacity: 0.3
        }
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
                color: palette.mid
                opacity: 0.3
            }
        }
    }
}

// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Cloud Sync Settings")

    // Push whatever is typed into the settings without waiting for the field to
    // lose focus. Tapping Back (or backgrounding the app) doesn't always blur
    // the active field first, so without this a value the user just typed would
    // be dropped — and the credential check on the way out would test the old
    // one. Main.qml calls this; committing an unchanged value is harmless
    // because AppController's setters ignore it.
    function _commitSyncFields() {
        AppController.syncEmail           = emailField.text
        AppController.syncPassword        = passwordField.text
        AppController.syncEncryptionPhrase = phraseField.text
    }

    // Same spot as on the list pages; see MainMenuPage.qml.
    NavButton {
        id: navButton
        anchors { top: parent.top; left: parent.left; topMargin: Theme.topInset + 8; leftMargin: 12 }
    }

    ScrollView {
        anchors.fill: parent    // content scrolls up under the title bar
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0
            Item { Layout.preferredHeight: navButton.y + navButton.height + 4 }

            // ── Sync toggle ───────────────────────────────────────────────────
            SettingsSection {
                title: qsTr("Sync")

                SettingsItem {
                    text: qsTr("Enable Sync")
                    showChevron: false
                    control: MonoSwitch {
                        checked: AppController.syncEnabled
                        onToggled: AppController.syncEnabled = checked
                    }
                }
            }

            // ── Credentials ───────────────────────────────────────────────────
            SettingsSection {
                title: qsTr("Account")

                SettingsItem {
                    text: qsTr("Email")
                    showChevron: false
                    fillControl: true
                    control: SettingsField {
                        id: emailField
                        text: AppController.syncEmail
                        placeholderText: qsTr("user@example.com")
                        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoPredictiveText
                        onEditingFinished: AppController.syncEmail = text
                    }
                }

                SettingsItem {
                    text: qsTr("Password")
                    showChevron: false
                    fillControl: true
                    control: SettingsField {
                        id: passwordField
                        text: AppController.syncPassword
                        placeholderText: qsTr("password")
                        echoMode: TextInput.Password
                        onEditingFinished: AppController.syncPassword = text
                    }
                }

                SettingsItem {
                    text: qsTr("Encryption Phrase")
                    showChevron: false
                    fillControl: true
                    control: SettingsField {
                        id: phraseField
                        text: AppController.syncEncryptionPhrase
                        placeholderText: qsTr("optional")
                        onEditingFinished: AppController.syncEncryptionPhrase = text
                    }
                }
            }

            // ── Subscription status ───────────────────────────────────────────
            SettingsSection {
                title: qsTr("Subscription")

                SettingsItem {
                    text: AppController.subscriptionStatusText.length > 0
                          ? AppController.subscriptionStatusText
                          : qsTr("Not connected — enable sync to view subscription status")
                    showChevron: false
                    down: false        // information only, not a button
                }

                SettingsItem {
                    text: qsTr("www.projectnotespro.com")
                    iconName: "safari"
                    showChevron: false
                    onClicked: Qt.openUrlExternally("https://www.projectnotespro.com")
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.bottomMargin: 24
                horizontalAlignment: Text.AlignHCenter
                text: AppController.supabaseConnectionInfo
                font.pixelSize: Theme.caption2; font.letterSpacing: Theme.tracking(Theme.caption2)
                color: Theme.secondaryText
            }
        }
    }
}

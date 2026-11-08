// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Full-screen main menu, opened from the ≡ button on the tab pages. It sits
// on the page stack like any other page, so its settings pages push on top
// of it and come back to it. Rows are grouped on rounded cards, as in the
// iOS Settings app (see SettingsSection / SettingsItem).

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Main Menu")

    property StackView stackView: null

    readonly property var _appearanceNames: [qsTr("System"), qsTr("Day"), qsTr("Night")]

    function _push(file) {
        if (root.stackView)
            root.stackView.push(Qt.resolvedUrl(file))
    }

    // Same spot as on the list pages, where it is centered on the 48px search
    // capsule that sits 6px from the top.
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

            SettingsSection {
                title: qsTr("App")

                SettingsItem {
                    text: qsTr("Appearance")
                    iconName: "circle.lefthalf.filled"
                    value: root._appearanceNames[AppController.appearance] || ""
                    onClicked: root._push("AppearancePage.qml")
                }
                SettingsItem {
                    text: qsTr("View")
                    iconName: "eye"
                    onClicked: root._push("ViewOptionsPage.qml")
                }
                SettingsItem {
                    text: qsTr("Preferences")
                    iconName: "gearshape"
                    onClicked: root._push("PreferencesPage.qml")
                }
            }

            SettingsSection {
                title: qsTr("Sync")

                SettingsItem {
                    text: qsTr("Cloud Sync Settings")
                    iconName: "icloud"
                    onClicked: root._push("SyncSettingsPage.qml")
                }
                // Back to the lists so the sync strip shows over what is syncing.
                SettingsItem {
                    text: qsTr("Sync All")
                    iconName: "arrow.triangle.2.circlepath"
                    showChevron: false
                    enabled: AppController.syncEnabled
                    onClicked: {
                        if (root.stackView)
                            root.stackView.pop()
                        AppController.syncAll()
                    }
                }
            }

            SettingsSection {
                title: qsTr("Support")

                SettingsItem {
                    text: qsTr("Help")
                    iconName: "questionmark.circle"
                    showChevron: false
                    onClicked: Qt.openUrlExternally("https://www.projectnotespro.com/docs/Mobile/ProjectNotesMobile")
                }
                SettingsItem {
                    text: qsTr("What's New")
                    iconName: "sparkles"
                    showChevron: false
                    onClicked: Qt.openUrlExternally("https://gitlab.com/kestermckinney/ProjectNotes/-/releases")
                }
                SettingsItem {
                    text: qsTr("About")
                    iconName: "info.circle"
                    onClicked: root._push("AboutPage.qml")
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }
}

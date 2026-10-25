// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("View")

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

            SettingsSection {
                title: qsTr("Projects")

                SettingsItem {
                    text: qsTr("Show Closed Projects")
                    showChevron: false
                    control: MonoSwitch {
                        checked: AppController.showClosedProjects
                        onToggled: {
                            AppController.showClosedProjects = checked
                            // A flick writes `checked` on the switch itself, so
                            // until the property next changes the row shows a value
                            // nothing has accepted yet. Re-assert the binding so it
                            // always ends up showing what the controller holds.
                            checked = Qt.binding(function() { return AppController.showClosedProjects })
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Tracker Items")

                SettingsItem {
                    text: qsTr("Internal Items")
                    showChevron: false
                    control: MonoSwitch {
                        checked: AppController.showInternalItems
                        onToggled: {
                            AppController.showInternalItems = checked
                            checked = Qt.binding(function() { return AppController.showInternalItems })
                        }
                    }
                }

                // Same name and same sense as the desktop app's View menu and its
                // View Options screen — the controller property is the inverse of
                // both, so this is the one row that binds through a negation.
                SettingsItem {
                    text: qsTr("Show Resolved Items")
                    showChevron: false
                    control: MonoSwitch {
                        checked: !AppController.newAndAssignedOnly
                        onToggled: {
                            AppController.newAndAssignedOnly = !checked
                            checked = Qt.binding(function() { return !AppController.newAndAssignedOnly })
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }
}

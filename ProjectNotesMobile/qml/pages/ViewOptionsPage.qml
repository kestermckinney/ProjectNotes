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
        anchors { top: parent.top; left: parent.left; topMargin: 8; leftMargin: 12 }
    }

    ScrollView {
        anchors { fill: parent; topMargin: navButton.y + navButton.height + 4 }
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            // ── Projects ──────────────────────────────────────────────────────
            SectionHeader { text: qsTr("Projects") }

            SettingsRow {
                label: qsTr("Show Closed Projects")
                control: Switch {
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

            // ── Tracker Items ─────────────────────────────────────────────────
            SectionHeader { text: qsTr("Tracker Items") }

            SettingsRow {
                label: qsTr("Internal Items")
                control: Switch {
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
            SettingsRow {
                label: qsTr("Show Resolved Items")
                control: Switch {
                    checked: !AppController.newAndAssignedOnly
                    onToggled: {
                        AppController.newAndAssignedOnly = !checked
                        checked = Qt.binding(function() { return !AppController.newAndAssignedOnly })
                    }
                }
            }
        }
    }

    // ── Shared helper components ──────────────────────────────────────────────

    component SectionHeader: Label {
        Layout.fillWidth: true
        Layout.topMargin: 10
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
        font.weight: 600
        color: Theme.sectionText
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
            color: "transparent"

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

        }
    }
}

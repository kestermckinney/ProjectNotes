// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("View")

    ScrollView {
        anchors.fill: parent
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
                    onToggled: AppController.showClosedProjects = checked
                }
            }

            // ── Tracker Items ─────────────────────────────────────────────────
            SectionHeader { text: qsTr("Tracker Items") }

            SettingsRow {
                label: qsTr("Internal Items")
                control: Switch {
                    checked: AppController.showInternalItems
                    onToggled: AppController.showInternalItems = checked
                }
            }

            SettingsRow {
                label: qsTr("New and Assigned Only")
                control: Switch {
                    checked: AppController.newAndAssignedOnly
                    onToggled: AppController.newAndAssignedOnly = checked
                }
            }
        }
    }

    // ── Shared helper components ──────────────────────────────────────────────

    component SectionHeader: Label {
        Layout.fillWidth: true
        Layout.topMargin: 20
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: 13
        font.weight: Font.Medium
        color: palette.mid
        background: Rectangle { color: palette.window }
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

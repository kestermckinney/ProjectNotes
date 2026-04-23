// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Preferences")

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            // ── Defaults ──────────────────────────────────────────────────────
            SectionHeader { text: qsTr("Managing Company") }
            FieldRow {
                ComboBox {
                    id: companyCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.clientsModel
                    textRole: "client_name"
                    Component.onCompleted: currentIndex = AppController.managingCompanyIndex()
                    onActivated: AppController.setManagingCompanyByRow(currentIndex)
                }
            }

            SectionHeader { text: qsTr("Project Manager") }
            FieldRow {
                ComboBox {
                    id: managerCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.peopleModel
                    textRole: "name"
                    Component.onCompleted: currentIndex = AppController.projectManagerIndex()
                    onActivated: AppController.setProjectManagerByRow(currentIndex)
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
        font.weight: 600
        color: Theme.navyMid
        background: Rectangle { color: Theme.sectionBg }
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
            color: palette.placeholderText
            opacity: 0.3
        }
    }
}

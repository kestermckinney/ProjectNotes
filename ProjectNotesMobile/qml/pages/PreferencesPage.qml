// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Preferences")

    // Stable {id,name} snapshots backing the combos below — deliberately NOT
    // AppController.clientsModel/peopleModel directly, which the Clients/
    // People tabs' Sort feature can reorder/reset out from under a ComboBox
    // bound live to them (see AppController::teamMemberList doc comment).
    property var _clients: []
    property var _people:  []
    function _clientNames() { return root._clients.map(function(c){ return c.name }) }
    function _peopleNames() { return root._people.map(function(p){ return p.name }) }
    function _indexForId(list, id) {
        for (var i = 0; i < list.length; i++) if (list[i].id === id) return i
        return -1
    }

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
                    model: root._clientNames()
                    Component.onCompleted: {
                        root._clients = AppController.clientList()
                        currentIndex = root._indexForId(root._clients, AppController.managingCompanyId())
                    }
                    onActivated: {
                        if (currentIndex >= 0 && currentIndex < root._clients.length)
                            AppController.setManagingCompanyId(root._clients[currentIndex].id)
                    }
                }
            }

            SectionHeader { text: qsTr("Project Manager") }
            FieldRow {
                ComboBox {
                    id: managerCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: root._peopleNames()
                    Component.onCompleted: {
                        root._people = AppController.peopleList()
                        currentIndex = root._indexForId(root._people, AppController.projectManagerId())
                    }
                    onActivated: {
                        if (currentIndex >= 0 && currentIndex < root._people.length)
                            AppController.setProjectManagerId(root._people[currentIndex].id)
                    }
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
}

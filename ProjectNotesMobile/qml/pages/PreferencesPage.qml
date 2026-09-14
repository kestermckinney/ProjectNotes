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
                FormCombo {
                    id: companyCombo
                    options: root._clientNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._clients = AppController.clientList()
                        selectOption(root._indexForId(root._clients, AppController.managingCompanyId()))
                    }
                    onActivated: {
                        var i = optionIndex
                        AppController.setManagingCompanyId(
                            (i >= 0 && i < root._clients.length) ? root._clients[i].id : "")
                    }
                }
            }

            SectionHeader { text: qsTr("Project Manager") }
            FieldRow {
                FormCombo {
                    id: managerCombo
                    options: root._peopleNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._people = AppController.peopleList()
                        selectOption(root._indexForId(root._people, AppController.projectManagerId()))
                    }
                    onActivated: {
                        var i = optionIndex
                        AppController.setProjectManagerId(
                            (i >= 0 && i < root._people.length) ? root._people[i].id : "")
                    }
                }
            }

            // ── Office Documents ─────────────────────────────────────────────
            SectionHeader { text: qsTr("Office Documents") }
            SettingsRow {
                label: qsTr("Open links in apps when available")
                control: Switch {
                    checked: AppController.office365OpenLinksInDesktop
                    onToggled: AppController.office365OpenLinksInDesktop = checked
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

    component SettingsRow: RowLayout {
        property string label: ""
        property alias control: controlSlot.children

        Layout.fillWidth: true
        Layout.minimumHeight: 44
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: palette.base

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16; topMargin: 8; bottomMargin: 8 }

                Label {
                    text: parent.parent.parent.label
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
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

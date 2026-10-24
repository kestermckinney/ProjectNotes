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
    // AppController.companiesModel/contactsModel directly, which the Companies/
    // Contacts tabs' Sort feature can reorder/reset out from under a ComboBox
    // bound live to them (see AppController::teamMemberList doc comment).
    property var _companies: []
    property var _contacts:  []
    function _companyNames() { return root._companies.map(function(c){ return c.name }) }
    function _contactNames() { return root._contacts.map(function(p){ return p.name }) }
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
                    options: root._companyNames()
                    includeNone: true
                    searchable: true
                    Component.onCompleted: {
                        root._companies = AppController.companyList()
                        selectOption(root._indexForId(root._companies, AppController.managingCompanyId()))
                    }
                    onActivated: {
                        var i = optionIndex
                        AppController.setManagingCompanyId(
                            (i >= 0 && i < root._companies.length) ? root._companies[i].id : "")
                    }
                }
            }

            SectionHeader { text: qsTr("Project Manager") }
            FieldRow {
                FormCombo {
                    id: managerCombo
                    options: root._contactNames()
                    includeNone: true
                    searchable: true
                    Component.onCompleted: {
                        root._contacts = AppController.contactList()
                        selectOption(root._indexForId(root._contacts, AppController.projectManagerId()))
                    }
                    onActivated: {
                        var i = optionIndex
                        AppController.setProjectManagerId(
                            (i >= 0 && i < root._contacts.length) ? root._contacts[i].id : "")
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
        Layout.minimumHeight: 44
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"

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

        }
    }
}

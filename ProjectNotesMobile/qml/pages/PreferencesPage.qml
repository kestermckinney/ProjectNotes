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

            // ── Defaults ──────────────────────────────────────────────────────
            SettingsSection {
                title: qsTr("Defaults")

                SettingsItem {
                    text: qsTr("Managing Company")
                    showChevron: false
                    fillControl: true
                    control: SettingsCombo {
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

                SettingsItem {
                    text: qsTr("Project Manager")
                    showChevron: false
                    fillControl: true
                    control: SettingsCombo {
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
            }

            // ── Office Documents ─────────────────────────────────────────────
            SettingsSection {
                title: qsTr("Office Documents")

                SettingsItem {
                    text: qsTr("Open links in apps when available")
                    showChevron: false
                    control: MonoSwitch {
                        checked: AppController.office365OpenLinksInDesktop
                        onToggled: AppController.office365OpenLinksInDesktop = checked
                    }
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }
}

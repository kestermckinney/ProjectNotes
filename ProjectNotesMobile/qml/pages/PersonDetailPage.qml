// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Person")

    property int    personRow:          -1
    property string personId:           ""
    property string initialName:        ""
    property string initialEmail:       ""
    property string initialOfficePhone: ""
    property string initialCellPhone:   ""
    property string initialClientId:    ""
    property string initialRole:        ""
    property bool   _skipSave:          false
    property bool   isNewRecord:        false

    // Stable {id,name} snapshot backing clientCombo — deliberately NOT the
    // live AppController.clientsModel proxy, which the Clients tab's Sort
    // feature can reorder/reset out from under a ComboBox bound directly to
    // it (see AppController::teamMemberList doc comment).
    property var _clients: []
    function _clientNames() { return root._clients.map(function(c){ return c.name }) }
    function _clientIndexForId(id) {
        for (var i = 0; i < root._clients.length; i++)
            if (root._clients[i].id === id) return i
        return -1
    }

    function _isBlankNew() { return isNewRecord && nameField.text.trim() === "" }
    function _discardNew()  {
        var row = AppController.rowForId(AppController.peopleModel, root.personId)
        if (row < 0) return
        AppController.deletePerson(row)
    }

    // Re-resolve personRow from the stable personId before every write —
    // Sort/refresh elsewhere in the app can reorder or reset the shared
    // peopleModel proxy while this page is open, which would otherwise leave
    // personRow pointing at a different (or no longer existing) record.
    function _saveNow() {
        var row = AppController.rowForId(AppController.peopleModel, root.personId)
        if (row < 0) return false   // record no longer exists
        root.personRow = row
        var ci = clientCombo.optionIndex
        var clientId = (ci >= 0 && ci < root._clients.length) ? root._clients[ci].id : ""
        return AppController.savePerson(root.personRow, nameField.text, emailField.text,
                                        officePhoneField.text, cellPhoneField.text,
                                        clientId, roleField.text)
    }

    function _reloadData() {
        var d = AppController.getPersonData(root.personRow)
        nameField.text        = (d.name         || "").toString()
        emailField.text       = (d.email        || "").toString()
        officePhoneField.text = (d.office_phone || "").toString()
        cellPhoneField.text   = (d.cell_phone   || "").toString()
        roleField.text        = (d.role         || "").toString()
        root._clients = AppController.clientList()
        clientCombo.selectOption(root._clientIndexForId((d.client_id || "").toString()))
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    Component.onDestruction: {
        root.forceActiveFocus()
        Qt.inputMethod.hide()
        if (!root._skipSave)
            root._saveNow()
    }

    // ── Toolbar: copy + delete ────────────────────────────────────────────────
    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            Item { Layout.fillWidth: true }

            ToolButton {
                icon.name: "doc.on.doc"
                onClicked: {
                    if (!root._saveNow()) return
                    root._skipSave = true
                    var newRow = AppController.copyPerson(root.personRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getPersonData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("PersonDetailPage.qml"), {
                        personRow:          newRow,
                        personId:           (d.id           || "").toString(),
                        initialName:        (d.name         || "").toString(),
                        initialEmail:       (d.email        || "").toString(),
                        initialOfficePhone: (d.office_phone || "").toString(),
                        initialCellPhone:   (d.cell_phone   || "").toString(),
                        initialClientId:    (d.client_id    || "").toString(),
                        initialRole:        (d.role         || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    var row = AppController.rowForId(AppController.peopleModel, root.personId)
                    if (row >= 0 && AppController.deletePerson(row)) {
                        root._skipSave = true
                        root.StackView.view.pop()
                    }
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            // ── Contact ───────────────────────────────────────────────────────

            SectionHeader { text: qsTr("Name") }
            FieldRow {
                FormField {
                    id: nameField
                    text: root.initialName
                    inputMethodHints: Qt.ImhNoPredictiveText
                }
            }

            SectionHeader { text: qsTr("Email") }
            FieldRow {
                FormField {
                    id: emailField
                    text: root.initialEmail
                    inputMethodHints: Qt.ImhEmailCharactersOnly
                }
            }

            SectionHeader { text: qsTr("Office Phone") }
            FieldRow {
                FormField {
                    id: officePhoneField
                    text: root.initialOfficePhone
                    inputMethodHints: Qt.ImhDialableCharactersOnly
                }
            }

            SectionHeader { text: qsTr("Cell Phone") }
            FieldRow {
                FormField {
                    id: cellPhoneField
                    text: root.initialCellPhone
                    inputMethodHints: Qt.ImhDialableCharactersOnly
                }
            }

            SectionHeader { text: qsTr("Role") }
            FieldRow {
                FormField {
                    id: roleField
                    text: root.initialRole
                    inputMethodHints: Qt.ImhNoPredictiveText
                }
            }

            // ── Company ───────────────────────────────────────────────────────

            SectionHeader { text: qsTr("Client") }
            FieldRow {
                FormCombo {
                    id: clientCombo
                    options: root._clientNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._clients = AppController.clientList()
                        selectOption(root._clientIndexForId(root.initialClientId))
                    }
                }
            }

            Item { Layout.preferredHeight: 24 }
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

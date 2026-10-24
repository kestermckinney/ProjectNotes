// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Contact")

    property int    contactRow:         -1
    property string contactId:          ""
    property string initialName:        ""
    property string initialEmail:       ""
    property string initialOfficePhone: ""
    property string initialCellPhone:   ""
    property string initialCompanyId:   ""
    property string initialRole:        ""
    property bool   _skipSave:          false
    property bool   isNewRecord:        false

    // Stable {id,name} snapshot backing companyCombo — deliberately NOT the
    // live AppController.companiesModel proxy, which the Companies tab's Sort
    // feature can reorder/reset out from under a ComboBox bound directly to
    // it (see AppController::teamMemberList doc comment).
    property var _companies: []
    function _companyNames() { return root._companies.map(function(c){ return c.name }) }
    function _companyIndexForId(id) {
        for (var i = 0; i < root._companies.length; i++)
            if (root._companies[i].id === id) return i
        return -1
    }

    function _isBlankNew() { return isNewRecord && nameField.text.trim() === "" }
    function _discardNew()  {
        var row = AppController.rowForId(AppController.contactsModel, root.contactId)
        if (row < 0) return
        AppController.deleteContact(row)
    }

    // Re-resolve contactRow from the stable contactId before every write —
    // Sort/refresh elsewhere in the app can reorder or reset the shared
    // contactsModel proxy while this page is open, which would otherwise leave
    // contactRow pointing at a different (or no longer existing) record.
    function _saveNow() {
        var row = AppController.rowForId(AppController.contactsModel, root.contactId)
        if (row < 0) return false   // record no longer exists
        root.contactRow = row
        var ci = companyCombo.optionIndex
        var companyId = (ci >= 0 && ci < root._companies.length) ? root._companies[ci].id : ""
        return AppController.saveContact(root.contactRow, nameField.text, emailField.text,
                                        officePhoneField.text, cellPhoneField.text,
                                        companyId, roleField.text)
    }

    function _reloadData() {
        var d = AppController.getContactData(root.contactRow)
        nameField.text        = (d.name         || "").toString()
        emailField.text       = (d.email        || "").toString()
        officePhoneField.text = (d.office_phone || "").toString()
        cellPhoneField.text   = (d.cell_phone   || "").toString()
        roleField.text        = (d.role         || "").toString()
        root._companies = AppController.companyList()
        companyCombo.selectOption(root._companyIndexForId((d.client_id || "").toString()))
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
    // ── Actions ──────────────────────────────────────────────────────────────
    // Glass pill floating at the top right, over the form as it scrolls.
    NavButton {
        id: navButton
        anchors { left: parent.left; leftMargin: 12; verticalCenter: topBar.verticalCenter }
    }

    GlassBar {
        id: topBar
        z: 2
        anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 12 }
        height: 44
        width: topBarRow.implicitWidth + 2 * contentPadding
        contentPadding: 6
        blurSource: scrollView
        blurRect: Qt.rect(topBar.x - scrollView.x, topBar.y - scrollView.y,
                          topBar.width, topBar.height)

        RowLayout {
            id: topBarRow
            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
            spacing: 0
            BarButton {
                icon.name: "doc.on.doc"
                onClicked: {
                    if (!root._saveNow()) return
                    root._skipSave = true
                    var newRow = AppController.copyContact(root.contactRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getContactData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("ContactDetailPage.qml"), {
                        contactRow:         newRow,
                        contactId:          (d.id           || "").toString(),
                        initialName:        (d.name         || "").toString(),
                        initialEmail:       (d.email        || "").toString(),
                        initialOfficePhone: (d.office_phone || "").toString(),
                        initialCellPhone:   (d.cell_phone   || "").toString(),
                        initialCompanyId:   (d.client_id    || "").toString(),
                        initialRole:        (d.role         || "").toString()
                    })
                }
            }

            BarButton {
                icon.name: "trash"
                onClicked: {
                    var row = AppController.rowForId(AppController.contactsModel, root.contactId)
                    if (row >= 0 && AppController.deleteContact(row)) {
                        root._skipSave = true
                        root.StackView.view.pop()
                    }
                }
            }
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0
            // Room for the floating action pill above the first field.
            Item { Layout.preferredHeight: topBar.height + 6 }


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

            SectionHeader { text: qsTr("Company") }
            FieldRow {
                FormCombo {
                    id: companyCombo
                    options: root._companyNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._companies = AppController.companyList()
                        selectOption(root._companyIndexForId(root.initialCompanyId))
                    }
                }
            }

            Item { Layout.preferredHeight: 24 }
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
}

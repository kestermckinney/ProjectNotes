// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Company")

    property int    companyRow:         -1
    property string companyId:          ""
    property string initialCompanyName: ""
    property bool   _skipSave:          false
    property bool   isNewRecord:        false

    function _isBlankNew() { return isNewRecord && companyNameField.text.trim() === "" }
    function _discardNew()  {
        var row = AppController.rowForId(AppController.companiesModel, root.companyId)
        if (row < 0) return
        AppController.deleteCompany(row)
    }

    // Re-resolve companyRow from the stable companyId before every write —
    // Sort/refresh elsewhere in the app can reorder or reset the shared
    // companiesModel proxy while this page is open.
    function _saveNow() {
        var row = AppController.rowForId(AppController.companiesModel, root.companyId)
        if (row < 0) return false   // record no longer exists
        root.companyRow = row
        return AppController.saveCompany(root.companyRow, companyNameField.text)
    }

    function _reloadData() {
        var d = AppController.getCompanyData(root.companyRow)
        companyNameField.text = (d.client_name || "").toString()
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
        anchors { top: parent.top; right: parent.right; topMargin: Theme.topInset + 6; rightMargin: 12 }
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
                    var newRow = AppController.copyCompany(root.companyRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getCompanyData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("CompanyDetailPage.qml"), {
                        companyRow:         newRow,
                        companyId:          (d.id           || "").toString(),
                        initialCompanyName: (d.client_name || "").toString()
                    })
                }
            }

            BarButton {
                icon.name: "trash"
                onClicked: {
                    var row = AppController.rowForId(AppController.companiesModel, root.companyId)
                    if (row >= 0 && AppController.deleteCompany(row)) {
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
            Item { Layout.preferredHeight: topBar.y + topBar.height + 6 }


            SectionHeader { text: qsTr("Company Name") }

            FieldRow {
                FormField {
                    id: companyNameField
                    text: root.initialCompanyName
                    inputMethodHints: Qt.ImhNoPredictiveText
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }
}

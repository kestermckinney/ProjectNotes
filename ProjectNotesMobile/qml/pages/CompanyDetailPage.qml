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

            ToolButton {
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
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            SectionHeader { text: qsTr("Company Name") }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: palette.base

                TextField {
                    id: companyNameField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialCompanyName
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }

                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1
                    color: Theme.mutedText
                    opacity: 0.3
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
        font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
        font.weight: 600
        color: Theme.navyMid
        background: Rectangle { color: Theme.sectionBg }
    }


}

// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Status Item")

    property int    itemRow:            -1
    property string itemId:             ""
    property string initialCategory:    ""
    property string initialDescription: ""
    property bool   _skipSave:          false
    property bool   _hasChanges:        false
    property bool   isNewRecord:        false

    function _isBlankNew() { return isNewRecord && descField.text.trim() === "" }
    function _discardNew()  {
        var row = AppController.rowForId(AppController.statusReportItemsModel, root.itemId)
        if (row < 0) return
        AppController.deleteStatusItem(row)
    }

    // Re-resolve itemRow from the stable itemId before every write — Sort/
    // refresh elsewhere in the app can reorder or reset the shared
    // statusReportItemsModel proxy while this page is open.
    function _saveNow() {
        if (!root._hasChanges) return true
        var row = AppController.rowForId(AppController.statusReportItemsModel, root.itemId)
        if (row < 0) return false   // record no longer exists
        root.itemRow = row
        var cat = categoryCombo.selection
        var result = AppController.saveStatusItem(root.itemRow, cat, descField.text)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getStatusItemData(root.itemRow)
        categoryCombo.selectText((d.task_category || "").toString(), 0)
        descField.text = (d.task_description || "").toString()
    }

    StackView.onDeactivating: {
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
                    var newRow = AppController.copyStatusItem(root.itemRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getStatusItemData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("StatusItemDetailPage.qml"), {
                        itemRow:            newRow,
                        itemId:             (d.id                || "").toString(),
                        initialCategory:    (d.task_category    || "").toString(),
                        initialDescription: (d.task_description || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    var row = AppController.rowForId(AppController.statusReportItemsModel, root.itemId)
                    if (row >= 0 && AppController.deleteStatusItem(row)) {
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

            SectionHeader { text: qsTr("Category") }
            FieldRow {
                FormCombo {
                    id: categoryCombo
                    options: AppController.statusItemCategoryOptions()
                    Component.onCompleted: selectText(root.initialCategory, 0)
                    onActivated: root._hasChanges = true
                }
            }

            SectionHeader { text: qsTr("Description") }
            FieldRow {
                Layout.preferredHeight: Math.max(100, descField.contentHeight + 24)
                TextArea {
                    id: descField
                    anchors { left: parent.left; right: parent.right; top: parent.top; bottom: parent.bottom; margins: 8 }
                    text: root.initialDescription
                    wrapMode: TextArea.Wrap
                    color: palette.text
                    background: Item {}
                    onTextChanged: root._hasChanges = true
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

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

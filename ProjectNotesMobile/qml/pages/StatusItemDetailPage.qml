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
    property string initialCategory:    ""
    property string initialDescription: ""
    property bool   _skipSave:          false
    property bool   _hasChanges:        false
    property bool   isNewRecord:        false

    function _isBlankNew() { return isNewRecord && descField.text.trim() === "" }
    function _discardNew()  { AppController.deleteStatusItem(root.itemRow) }

    function _saveNow() {
        if (!root._hasChanges) return true
        var cat = (categoryCombo.currentIndex >= 0)
            ? categoryCombo.model[categoryCombo.currentIndex] : ""
        var result = AppController.saveStatusItem(root.itemRow, cat, descField.text)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getStatusItemData(root.itemRow)
        var ci = categoryCombo.model.indexOf((d.task_category || "").toString())
        categoryCombo.currentIndex = ci >= 0 ? ci : 0
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
                        initialCategory:    (d.task_category    || "").toString(),
                        initialDescription: (d.task_description || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deleteStatusItem(root.itemRow)
                    root.StackView.view.pop()
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
                ComboBox {
                    id: categoryCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.statusItemCategoryOptions()
                    Component.onCompleted: {
                        var idx = model.indexOf(root.initialCategory)
                        currentIndex = (idx >= 0) ? idx : 0
                    }
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

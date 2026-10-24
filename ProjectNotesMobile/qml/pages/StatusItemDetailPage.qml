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

            BarButton {
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
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0
            // Room for the floating action pill above the first field.
            Item { Layout.preferredHeight: topBar.height + 6 }


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
                Layout.preferredHeight: Math.max(100, descField.contentHeight + 32)
                TextArea {
                    id: descField
                    anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 6; bottomMargin: 6 }
                    padding: 10
                    text: root.initialDescription
                    wrapMode: TextArea.Wrap
                    color: palette.text
                    background: GlassField { focused: descField.activeFocus }
                    onTextChanged: root._hasChanges = true
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

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

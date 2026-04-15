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

    function _saveNow() {
        var cat = (categoryCombo.currentIndex >= 0)
            ? categoryCombo.model[categoryCombo.currentIndex] : ""
        AppController.saveStatusItem(root.itemRow, cat, descField.text)
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
                    root._saveNow()
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
        font.weight: Font.Semibold
        color: "#0A7AFF"
        background: Rectangle { color: Qt.rgba(10/255, 122/255, 255/255, 0.06) }
    }

    component FieldRow: Rectangle {
        default property alias content: innerItem.data
        Layout.fillWidth: true
        Layout.preferredHeight: 44
        color: palette.base
        Item { id: innerItem; anchors.fill: parent }
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
            height: 1; color: palette.mid; opacity: 0.3
        }
    }
}

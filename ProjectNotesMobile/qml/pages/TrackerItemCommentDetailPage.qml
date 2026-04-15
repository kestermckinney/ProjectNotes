// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// TrackerItemCommentDetailPage — view/edit a single tracker item comment.
// Columns: 0=id, 1=item_id, 2=lastupdated_date, 3=update_note, 4=updated_by

Page {
    id: root
    title: qsTr("Comment")

    property int    commentRow:  -1
    property string initialDate: ""
    property string initialNote: ""
    property string initialBy:   ""
    property bool   _skipSave:   false

    function _saveNow() {
        var byId = updatedByCombo.currentIndex >= 0
            ? AppController.peopleIdAtRow(updatedByCombo.currentIndex) : ""
        AppController.saveComment(root.commentRow, dateField.text, noteEdit.text, byId)
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

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
                    var newRow = AppController.copyComment(root.commentRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getCommentData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("TrackerItemCommentDetailPage.qml"), {
                        commentRow:    newRow,
                        initialDate:   (d.lastupdated_date || "").toString(),
                        initialNote:   (d.update_note      || "").toString(),
                        initialBy:     (d.updated_by       || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deleteComment(root.commentRow)
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

            SectionHeader { text: qsTr("Date") }
            DateFieldRow { id: dateField; text: root.initialDate }

            SectionHeader { text: qsTr("Updated By") }
            FieldRow {
                ComboBox {
                    id: updatedByCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.peopleModel
                    textRole: "name"
                    Component.onCompleted: {
                        var row = AppController.peopleRowForId(root.initialBy)
                        currentIndex = row >= 0 ? row : -1
                    }
                }
            }

            SectionHeader { text: qsTr("Note") }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(200, noteEdit.contentHeight + 24)
                color: palette.base

                TextEdit {
                    id: noteEdit
                    anchors { fill: parent; margins: 8 }
                    text: root.initialNote
                    wrapMode: TextEdit.Wrap
                    color: palette.text
                    selectByMouse: true
                }
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1; color: palette.mid; opacity: 0.3
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
        font.weight: Font.Medium
        color: palette.mid
        background: Rectangle { color: palette.window }
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

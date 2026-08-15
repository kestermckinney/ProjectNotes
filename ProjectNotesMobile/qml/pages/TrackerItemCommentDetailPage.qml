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
    property bool   _hasChanges: false
    property bool   isNewRecord: false

    // Stable {id,name} snapshot backing updatedByCombo — deliberately NOT the
    // live AppController.peopleModel proxy, which the People tab's Sort
    // feature can reorder/reset out from under a ComboBox bound directly to
    // it (see AppController::teamMemberList doc comment). All people, not
    // just this project's team, matches desktop's Updated By field.
    property var _people: []
    function _peopleNames() { return root._people.map(function(p){ return p.name }) }
    function _personIndexForId(id) {
        for (var i = 0; i < root._people.length; i++)
            if (root._people[i].id === id) return i
        return -1
    }

    function _isBlankNew() { return isNewRecord && noteEdit.text.trim() === "" }
    function _discardNew()  { AppController.deleteComment(root.commentRow) }

    function _saveNow() {
        if (!root._hasChanges) return true
        dateField.commitPending()
        var byId = (updatedByCombo.currentIndex >= 0 && updatedByCombo.currentIndex < root._people.length)
            ? root._people[updatedByCombo.currentIndex].id : ""
        var result = AppController.saveComment(root.commentRow, dateField.text, noteEdit.text, byId)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getCommentData(root.commentRow)
        dateField.text = (d.lastupdated_date || "").toString()
        noteEdit.text  = (d.update_note      || "").toString()
        root._people = AppController.peopleList()
        updatedByCombo.currentIndex = root._personIndexForId((d.updated_by || "").toString())
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
                    if (!root._saveNow()) return
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
                    if (AppController.deleteComment(root.commentRow)) {
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

            SectionHeader { text: qsTr("Date") }
            DateFieldRow { id: dateField; text: root.initialDate; onTextChanged: root._hasChanges = true }

            SectionHeader { text: qsTr("Updated By") }
            FieldRow {
                ComboBox {
                    id: updatedByCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: root._peopleNames()
                    Component.onCompleted: {
                        root._people = AppController.peopleList()
                        currentIndex = root._personIndexForId(root.initialBy)
                    }
                    onActivated: root._hasChanges = true
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
                    onTextChanged: root._hasChanges = true
                }
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1; color: Theme.mutedText; opacity: 0.3
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

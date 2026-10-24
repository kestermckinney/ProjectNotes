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
    // live AppController.contactsModel proxy, which the Contacts tab's Sort
    // feature can reorder/reset out from under a ComboBox bound directly to
    // it (see AppController::teamMemberList doc comment). All contacts, not
    // just this project's team, matches desktop's Updated By field.
    property var _contacts: []
    function _contactNames() { return root._contacts.map(function(p){ return p.name }) }
    function _contactIndexForId(id) {
        for (var i = 0; i < root._contacts.length; i++)
            if (root._contacts[i].id === id) return i
        return -1
    }

    function _isBlankNew() { return isNewRecord && noteEdit.text.trim() === "" }
    function _discardNew()  { AppController.deleteComment(root.commentRow) }

    function _saveNow() {
        if (!root._hasChanges) return true
        dateField.commitPending()
        var bi = updatedByCombo.optionIndex
        var byId = (bi >= 0 && bi < root._contacts.length) ? root._contacts[bi].id : ""
        var result = AppController.saveComment(root.commentRow, dateField.text, noteEdit.text, byId)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getCommentData(root.commentRow)
        dateField.text = (d.lastupdated_date || "").toString()
        noteEdit.text  = (d.update_note      || "").toString()
        root._contacts = AppController.contactList()
        updatedByCombo.selectOption(root._contactIndexForId((d.updated_by || "").toString()))
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

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

            BarButton {
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
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0
            // Room for the floating action pill above the first field.
            Item { Layout.preferredHeight: topBar.height + 6 }


            SectionHeader { text: qsTr("Date") }
            DateFieldRow { id: dateField; text: root.initialDate; onTextChanged: root._hasChanges = true }

            SectionHeader { text: qsTr("Updated By") }
            FieldRow {
                FormCombo {
                    id: updatedByCombo
                    options: root._contactNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._contacts = AppController.contactList()
                        selectOption(root._contactIndexForId(root.initialBy))
                    }
                    onActivated: root._hasChanges = true
                }
            }

            SectionHeader { text: qsTr("Note") }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(200, noteEdit.contentHeight + 40)
                color: "transparent"

                GlassField {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 6; bottomMargin: 6 }
                    focused: noteEdit.activeFocus
                }

                TextEdit {
                    id: noteEdit
                    anchors { fill: parent; leftMargin: 22; rightMargin: 22; topMargin: 14; bottomMargin: 14 }
                    text: root.initialNote
                    wrapMode: TextEdit.Wrap
                    color: palette.text
                    selectByMouse: true
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

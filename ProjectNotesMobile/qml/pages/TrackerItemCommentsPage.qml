// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// TrackerItemCommentsPage — list of comments for a single tracker item.
// Columns from trackeritemcommentsmodel.cpp:
//   0=id, 1=item_id, 2=lastupdated_date, 3=update_note, 4=updated_by

Page {
    id: root
    title: {
        var base = qsTr("Comments")
        var num = root.itemNumber.trim()
        var name = root.itemName.trim().substring(0, 20)
        return base + " - " + num + " " + name
    }

    property string itemId: ""
    property string itemNumber: ""
    property string itemName: ""

    StackView.onActivated: AppController.openTrackerItem(root.itemId)

    header: ToolBar {
        // Glass search capsule, matching the floating tab bar. The bar
        // itself takes the page color so no band shows above the list.
        background: Rectangle { color: Theme.pageBackground }
        z: 1   // let the capsule's shadow fall over the list
        implicitHeight: 60

        GlassBar {
            anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 6; bottomMargin: 6 }
            contentPadding: 0

            RowLayout {
                anchors { fill: parent; leftMargin: 4; rightMargin: 2 }
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search comments…")
                    onTextChanged: AppController.setQuickSearch(AppController.trackerItemCommentsModel, text)
                    inputMethodHints: Qt.ImhNoPredictiveText
                    leftPadding: 10
                    rightPadding: clearBtn.visible ? clearBtn.width + 12 : 10
                    implicitHeight: 36
                    background: Item {}

                    Label {
                        id: clearBtn
                        visible: searchField.text.length > 0
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 10 }
                        text: "✕"
                        font.pixelSize: 18
                        color: palette.text
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -6
                            onClicked: searchField.clear()
                        }
                    }
                }
                ToolButton {
                    icon.name: "plus"
                    onClicked: {
                        var newRow = AppController.addComment(root.itemId)
                        if (newRow < 0) return
                        var d = AppController.getCommentData(newRow)
                        root.StackView.view.push(Qt.resolvedUrl("TrackerItemCommentDetailPage.qml"), {
                            commentRow:    newRow,
                            isNewRecord:   true,
                            initialDate:   (d.lastupdated_date || "").toString(),
                            initialNote:   (d.update_note      || "").toString(),
                            initialBy:     (d.updated_by       || "").toString()
                        })
                    }
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.trackerItemCommentsModel
        clip: true

        delegate: ItemDelegate {
            id: delegateRoot
            required property int index
            required property var model
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: delegateRoot.model.lastupdated_date || ""
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: AppController.contactNameForId(delegateRoot.model.updated_by || "")
                        font.pixelSize: Theme.caption1; font.letterSpacing: Theme.tracking(Theme.caption1)
                        color: Theme.mutedText
                    }
                }

                Label {
                    visible: (delegateRoot.model.update_note || "") !== ""
                    text: delegateRoot.model.update_note || ""
                    font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("TrackerItemCommentDetailPage.qml"), {
                    commentRow:    delegateRoot.index,
                    initialDate:   delegateRoot.model.lastupdated_date || "",
                    initialNote:   delegateRoot.model.update_note      || "",
                    initialBy:     delegateRoot.model.updated_by       || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No comments.")
        color: Theme.mutedText
    }
}

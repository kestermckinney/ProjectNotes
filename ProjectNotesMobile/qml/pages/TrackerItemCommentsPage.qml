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
    title: qsTr("Comments")

    property string itemId: ""

    StackView.onActivated: AppController.openTrackerItem(root.itemId)

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search comments…")
                onTextChanged: AppController.setQuickSearch(AppController.trackerItemCommentsModel, text)
                inputMethodHints: Qt.ImhNoPredictiveText
                rightPadding: clearBtn.visible ? clearBtn.width + 4 : 0

                Label {
                    id: clearBtn
                    visible: searchField.text.length > 0
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 6 }
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
                        initialDate:   (d.lastupdated_date || "").toString(),
                        initialNote:   (d.update_note      || "").toString(),
                        initialBy:     (d.updated_by       || "").toString()
                    })
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
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: model.lastupdated_date || ""
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: AppController.peopleNameForId(model.updated_by || "")
                        font.pixelSize: 12
                        color: palette.placeholderText
                    }
                }

                Label {
                    visible: (model.update_note || "") !== ""
                    text: model.update_note || ""
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("TrackerItemCommentDetailPage.qml"), {
                    commentRow:    index,
                    initialDate:   model.lastupdated_date || "",
                    initialNote:   model.update_note      || "",
                    initialBy:     model.updated_by       || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No comments.")
        color: palette.placeholderText
    }
}

// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// ProjectNotesPage — meeting notes for the current project.
// Mirrors the Notes tab in the desktop project details view.
// Columns from projectnotesmodel.cpp:
//   0=id, 1=project_id, 2=note_title, 3=note_date, 4=note, 5=internal_item

Page {
    id: root
    title: qsTr("Notes")

    property string projectId:    ""
    property string projectTitle: ""

    StackView.onActivated: AppController.refreshProjectNotes()

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search notes…")
                onTextChanged: AppController.setQuickSearch(AppController.projectNotesModel, text)
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
                    var newRow = AppController.addProjectNote(root.projectId)
                    if (newRow < 0) return
                    var d = AppController.getProjectNoteData(newRow)
                    var newId = (d.id || "").toString()
                    root.StackView.view.push(Qt.resolvedUrl("ProjectNoteDetailPage.qml"), {
                        noteRow:         newRow,
                        noteId:          newId,
                        projectId:       root.projectId,
                        initialTitle:    (d.note_title    || "").toString(),
                        initialDate:     (d.note_date     || "").toString(),
                        initialNote:     (d.note          || "").toString(),
                        initialInternal: (d.internal_item || "0") !== "0"
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.projectNotesModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: model.note_title || qsTr("(untitled)")
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: model.note_date || ""
                        font.pixelSize: 12
                        color: palette.placeholderText
                    }
                }

                Label {
                    visible: AppController.showInternalItems && (model.internal_item || "0") !== "0"
                    text: qsTr("Internal")
                    font.pixelSize: 11
                    color: palette.link
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("ProjectNoteDetailPage.qml"), {
                    noteRow:          index,
                    noteId:           model.id            || "",
                    projectId:        root.projectId,
                    initialTitle:     model.note_title    || "",
                    initialDate:      model.note_date     || "",
                    initialNote:      model.note          || "",
                    initialInternal:  (model.internal_item || "0") !== "0"
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Column {
        anchors.centerIn: parent
        visible: listView.count === 0
        spacing: 10

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "\uD83D\uDCDD"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No Notes")
            font.pixelSize: 17
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add a meeting note.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: palette.placeholderText
        }
    }
}

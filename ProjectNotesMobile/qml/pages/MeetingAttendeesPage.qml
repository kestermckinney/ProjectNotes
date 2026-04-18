// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// MeetingAttendeesPage — attendees list for a single meeting note.
// Columns from meetingattendeesmodel.cpp:
//   0=id, 1=note_id, 2=person_id, 3=name, 4=project_name,
//   5=email, 6=client_name, 7=project_id, 8=project_number

Page {
    id: root
    title: qsTr("Attendees")

    property string noteId: ""

    StackView.onActivated: AppController.refreshMeetingAttendees()

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search attendees…")
                onTextChanged: AppController.setQuickSearch(AppController.meetingAttendeesModel, text)
                inputMethodHints: Qt.ImhNoPredictiveText
            }
            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addAttendee(root.noteId)
                    if (newRow < 0) return
                    var d = AppController.getAttendeeData(newRow)
                    root.StackView.view.push(Qt.resolvedUrl("MeetingAttendeeDetailPage.qml"), {
                        attendeeRow:  newRow,
                        initialPerson:(d.person_id || "").toString()
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.meetingAttendeesModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                Label {
                    text: model.name || ""
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        visible: (model.client_name || "") !== ""
                        text: model.client_name || ""
                        font.pixelSize: 12
                        color: palette.placeholderText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: (model.email || "") !== ""
                        text: model.email || ""
                        font.pixelSize: 12
                        color: palette.placeholderText
                        elide: Text.ElideRight
                    }
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("MeetingAttendeeDetailPage.qml"), {
                    attendeeRow:  index,
                    initialPerson: model.person_id || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No attendees.")
        color: palette.placeholderText
    }
}

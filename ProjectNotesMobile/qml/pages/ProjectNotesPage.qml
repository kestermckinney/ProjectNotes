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
    title: {
        var base = qsTr("Notes")
        if (root.projectId === "") return base
        return base + " — " + AppController.projectNumberForId(root.projectId)
                    + " " + AppController.projectNameForId(root.projectId).substring(0, 20)
    }

    property string projectId:    ""
    property string projectTitle: ""

    StackView.onActivated: AppController.refreshProjectNotes()

    Connections {
        target: AppController
        function onViewOptionsChanged() {
            if (root.StackView.status !== StackView.Active) return
            AppController.refreshProjectNotes()
        }
    }

    FilterSheet { id: filterSheet }
    SortSheet   { id: sortSheet }

    // ── Search bar ─────────────────────────────────────────────────────────
    // A glass capsule floating over the top of the list. The rows scroll
    // beneath it and show through frosted, as they do under the tab bar.
    NavButton {
        id: navButton
        anchors { left: parent.left; leftMargin: 12; verticalCenter: searchBar.verticalCenter }
    }

    GlassBar {
        id: searchBar
        z: 2
        anchors { top: parent.top; left: navButton.right; right: parent.right; topMargin: Theme.topInset + 6; leftMargin: 8; rightMargin: 12 }
        height: 48
        contentPadding: 0
        blurSource: listView
        blurRect: Qt.rect(searchBar.x - listView.x, searchBar.y - listView.y,
                          searchBar.width, searchBar.height)

        RowLayout {
            anchors { fill: parent; leftMargin: 4; rightMargin: 2 }
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search notes…")
                onTextChanged: AppController.setQuickSearch(AppController.projectNotesModel, text)
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
            BarButton {
                icon.name: "line.3.horizontal.decrease.circle"
                icon.color: filterBadge.iconColor
                onClicked: filterSheet.openFor("notes", qsTr("Notes"))
                ActiveIndicator {
                    id: filterBadge
                    active: AppController.filterRev >= 0 && AppController.hasActiveColumnFilters(AppController.projectNotesModel)
                }
            }
            BarButton {
                icon.name: "arrow.up.arrow.down"
                icon.color: sortBadge.iconColor
                onClicked: sortSheet.openFor("notes", qsTr("Notes"))
                ActiveIndicator {
                    id: sortBadge
                    active: AppController.sortRev >= 0 && (AppController.activeSort(AppController.projectNotesModel).field || "") !== ""
                }
            }
            BarButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addProjectNote(root.projectId)
                    if (newRow < 0) return
                    var d = AppController.getProjectNoteData(newRow)
                    var newId = (d.id || "").toString()
                    root.StackView.view.push(Qt.resolvedUrl("ProjectNoteDetailPage.qml"), {
                        noteRow:         newRow,
                        isNewRecord:     true,
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
        topMargin: searchBar.y + searchBar.height + 6   // first row clear of the title bar and search capsule
        // The lists exist before the title bar is measured; start them at the top
        // once the margin settles rather than with the first row underneath it.
        onTopMarginChanged: contentY = -topMargin
        anchors.fill: parent
        model: AppController.projectNotesModel
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
                        text: delegateRoot.model.note_title || qsTr("(untitled)")
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: delegateRoot.model.note_date || ""
                        font.pixelSize: Theme.caption1; font.letterSpacing: Theme.tracking(Theme.caption1)
                        color: Theme.mutedText
                    }
                }

                Label {
                    visible: AppController.showInternalItems && (delegateRoot.model.internal_item || "0") !== "0"
                    text: qsTr("Internal")
                    font.pixelSize: Theme.caption2; font.letterSpacing: Theme.tracking(Theme.caption2)
                    color: palette.link
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("ProjectNoteDetailPage.qml"), {
                    noteRow:          delegateRoot.index,
                    noteId:           delegateRoot.model.id            || "",
                    projectId:        root.projectId,
                    initialTitle:     delegateRoot.model.note_title    || "",
                    initialDate:      delegateRoot.model.note_date     || "",
                    initialNote:      delegateRoot.model.note          || "",
                    initialInternal:  (delegateRoot.model.internal_item || "0") !== "0"
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
            font.pixelSize: Theme.title2; font.letterSpacing: Theme.tracking(Theme.title2)
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add a meeting note.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.subheadline; font.letterSpacing: Theme.tracking(Theme.subheadline)
            color: Theme.mutedText
        }
    }
}

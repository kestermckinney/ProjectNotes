// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// ProjectLocationsPage — files & folders for the current project.
// Mirrors the Locations tab in the desktop project details view.
// Columns from projectlocationsmodel.cpp:
//   0=id, 1=project_id, 2=location_type, 3=location_description, 4=full_path

Page {
    id: root
    title: {
        var base = qsTr("Files & Folders")
        if (root.projectId === "") return base
        return base + " — " + AppController.projectNumberForId(root.projectId)
                    + " " + AppController.projectNameForId(root.projectId).substring(0, 20)
    }

    property string projectId:    ""
    property string projectTitle: ""

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
                placeholderText: qsTr("Search files…")
                onTextChanged: AppController.setQuickSearch(AppController.projectLocationsModel, text)
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
                onClicked: filterSheet.openFor("locations", qsTr("Files & Folders"))
                ActiveIndicator {
                    id: filterBadge
                    active: AppController.filterRev >= 0 && AppController.hasActiveColumnFilters(AppController.projectLocationsModel)
                }
            }
            BarButton {
                icon.name: "arrow.up.arrow.down"
                icon.color: sortBadge.iconColor
                onClicked: sortSheet.openFor("locations", qsTr("Files & Folders"))
                ActiveIndicator {
                    id: sortBadge
                    active: AppController.sortRev >= 0 && (AppController.activeSort(AppController.projectLocationsModel).field || "") !== ""
                }
            }
            BarButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addProjectLocation(root.projectId)
                    if (newRow < 0) return
                    var d = AppController.getProjectLocationData(newRow)
                    root.StackView.view.push(Qt.resolvedUrl("ProjectLocationDetailPage.qml"), {
                        locationRow:         newRow,
                        locationId:          (d.id                    || "").toString(),
                        isNewRecord:         true,
                        initialType:         (d.location_type        || "").toString(),
                        initialDescription:  (d.location_description || "").toString(),
                        initialPath:         (d.full_path            || "").toString()
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
        model: AppController.projectLocationsModel
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
                        text: delegateRoot.model.location_description || delegateRoot.model.full_path || ""
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: delegateRoot.model.location_type || ""
                        font.pixelSize: Theme.caption1; font.letterSpacing: Theme.tracking(Theme.caption1)
                        color: Theme.mutedText
                    }
                }

                Label {
                    visible: (delegateRoot.model.full_path || "") !== "" && (delegateRoot.model.location_description || "") !== ""
                    text: delegateRoot.model.full_path || ""
                    font.pixelSize: Theme.caption1; font.letterSpacing: Theme.tracking(Theme.caption1)
                    color: Theme.mutedText
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("ProjectLocationDetailPage.qml"), {
                    locationRow:         delegateRoot.index,
                    locationId:          delegateRoot.model.id                  || "",
                    initialType:         delegateRoot.model.location_type        || "",
                    initialDescription:  delegateRoot.model.location_description || "",
                    initialPath:         delegateRoot.model.full_path            || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Label {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: qsTr("No files or folders.")
        color: Theme.mutedText
    }
}

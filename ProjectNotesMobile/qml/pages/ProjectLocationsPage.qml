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

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search files…")
                onTextChanged: AppController.setQuickSearch(AppController.projectLocationsModel, text)
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
                icon.name: "line.3.horizontal.decrease.circle"
                icon.color: filterBadge.iconColor
                onClicked: filterSheet.openFor("locations", qsTr("Files & Folders"))
                ActiveIndicator {
                    id: filterBadge
                    active: AppController.filterRev >= 0 && AppController.hasActiveColumnFilters(AppController.projectLocationsModel)
                }
            }
            ToolButton {
                icon.name: "arrow.up.arrow.down"
                icon.color: sortBadge.iconColor
                onClicked: sortSheet.openFor("locations", qsTr("Files & Folders"))
                ActiveIndicator {
                    id: sortBadge
                    active: AppController.sortRev >= 0 && (AppController.activeSort(AppController.projectLocationsModel).field || "") !== ""
                }
            }
            ToolButton {
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
                        font.pixelSize: 12
                        color: Theme.mutedText
                    }
                }

                Label {
                    visible: (delegateRoot.model.full_path || "") !== "" && (delegateRoot.model.location_description || "") !== ""
                    text: delegateRoot.model.full_path || ""
                    font.pixelSize: 12
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

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// "Move To Folder" picker for a project: every user folder plus "Not
// Categorized" listed as a single clickable row. Tapping one immediately
// replaces the project's folder memberships with that single choice (or
// clears them, for "Not Categorized") and closes — no separate confirm step.
// Multi-folder membership is still available by dragging a project onto a
// second folder in the sidebar; this dialog is the one-click "put this
// project in exactly one folder" affordance requested from the row menu.
Dialog {
    id: dlg
    // Popups auto-parent to the window overlay only when explicitly told to
    // (see RecordContextMenu's comment on the same pattern) — this dialog is
    // instantiated inside the narrow sidebar column as well as the full-width
    // projects list, so it must not inherit either one's bounds.
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 270
    height: content.implicitHeight + topPadding + bottomPadding
    modal: true; padding: 0
    scale: Theme.uiScale
    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

    property string _projectId: ""
    property string _label: ""

    function openFor(projectId, label) {
        dlg._projectId = projectId
        dlg._label = label || ""
        open()
    }

    // folderId === "" moves the project to Not Categorized (clears every
    // membership); otherwise it becomes the project's sole folder.
    function _pick(folderId) {
        FolderManager.removeProjectFromAllFolders(dlg._projectId)
        if (folderId !== "")
            FolderManager.addProjectToFolder(dlg._projectId, folderId)
        dlg.close()
    }

    contentItem: ColumnLayout {
        id: content
        spacing: 0
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 12
            Text { text: qsTr("Move To Folder"); color: Theme.text; font.pixelSize: 14; font.weight: Font.Bold; Layout.fillWidth: true }
            MaterialIcon { name: "close"; size: 18; color: Theme.text3; TapHandler { onTapped: dlg.close() } }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        Text {
            visible: dlg._label !== ""
            text: dlg._label
            color: Theme.text3
            font.pixelSize: 10
            Layout.fillWidth: true
            Layout.leftMargin: 13; Layout.rightMargin: 13; Layout.topMargin: 8
            elide: Text.ElideRight
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 2

            Repeater {
                model: FolderManager.folders
                delegate: Rectangle {
                    id: folderRow
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: 32
                    radius: Theme.radiusSm
                    color: rowHover.hovered ? Theme.surface2 : "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 7; anchors.rightMargin: 7
                        spacing: 8
                        MaterialIcon { name: folderRow.modelData.icon; size: 14; color: folderRow.modelData.color }
                        Text {
                            text: folderRow.modelData.name
                            color: Theme.text; font.pixelSize: 12
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                        Text { text: folderRow.modelData.count; color: Theme.text3; font.pixelSize: 10 }
                    }
                    HoverHandler { id: rowHover }
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dlg._pick(folderRow.modelData.id) }
                }
            }

            Text {
                visible: FolderManager.folders.length === 0
                text: qsTr("No folders yet — add one in Settings.")
                color: Theme.text3
                font.pixelSize: 11
                Layout.margins: 5
            }

            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft
                Layout.topMargin: 3; Layout.bottomMargin: 3
            }

            Rectangle {
                id: uncatRow
                Layout.fillWidth: true
                implicitHeight: 32
                radius: Theme.radiusSm
                color: uncatHover.hovered ? Theme.surface2 : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 7; anchors.rightMargin: 7
                    spacing: 8
                    MaterialIcon { name: "folder_off"; size: 14; color: Theme.text3 }
                    Text { text: qsTr("Not Categorized"); color: Theme.text; font.pixelSize: 12; Layout.fillWidth: true }
                }
                HoverHandler { id: uncatHover }
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dlg._pick("") }
            }
        }
    }
}

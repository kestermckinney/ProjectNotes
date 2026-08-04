// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Project picker behind a tracker item's "Move To…" action — the non-drag
// alternative to dragging the item onto a sidebar project row. Modeled on
// ProjectDetailPage's teamPicker Dialog (search field + filtered ListView).
// Picking a project doesn't move the item itself: it emits projectPicked(),
// and Main.requestTrackerItemMove() runs the same check-then-maybe-review
// flow the drag/drop path uses, so both entry points behave identically.
Dialog {
    id: dlg
    anchors.centerIn: parent
    width: 360; height: 420; modal: true; padding: 0
    scale: Theme.uiScale   // match the zoomed workspace (centered origin)
    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

    property string _itemId: ""
    property string _excludeProjectId: ""
    property string _filter: ""

    signal projectPicked(string itemId, string projectId)

    // Open for a given tracker item: excludes its current project from the
    // list (the check/move flow already no-ops a same-project pick, but
    // leaving it out of the list is clearer).
    function openFor(itemId) {
        dlg._itemId = itemId
        DesktopAppController.openTrackerItem(itemId)
        var d = DesktopAppController.getTrackerItemDetailData(0)
        dlg._excludeProjectId = (d.project_id || "").toString()
        open()
    }

    onOpened: { _filter = ""; projSearch.text = ""; projSearch.forceActiveFocus() }

    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 14
            Text { text: qsTr("Move To Project"); color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold; Layout.fillWidth: true }
            MaterialIcon { name: "close"; size: 20; color: Theme.text3; TapHandler { onTapped: dlg.close() } }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        // Search field — filters the list below as you type.
        Rectangle {
            Layout.fillWidth: true
            Layout.margins: 12
            implicitHeight: 34
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: projSearch.activeFocus ? Theme.accent : Theme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10; anchors.rightMargin: 10
                spacing: 6
                MaterialIcon { name: "search"; size: 16; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                TextField {
                    id: projSearch
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search projects…")
                    placeholderTextColor: Theme.text3
                    color: Theme.text
                    font.pixelSize: 13
                    background: null
                    verticalAlignment: Text.AlignVCenter
                    selectByMouse: true
                    onTextChanged: dlg._filter = text
                }
            }
        }

        ListView {
            id: projList
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            model: DesktopAppController.projectsListModel
            delegate: ItemDelegate {
                id: projDelegate
                required property int index
                required property var model
                readonly property string pid: model.id !== undefined ? model.id.toString() : ""
                readonly property string pname:
                    ((model.project_number || "") + "  " + (model.project_name || "")).trim()
                // Collapse rows that don't contain the search text, and the item's
                // own current project (moving it there would be a no-op).
                readonly property bool _match: pid !== dlg._excludeProjectId
                    && (dlg._filter === "" || pname.toLowerCase().indexOf(dlg._filter.toLowerCase()) >= 0)
                visible: _match
                width: projList.width; height: _match ? 40 : 0
                contentItem: Text { text: projDelegate.pname; color: Theme.text; font.pixelSize: 13; leftPadding: 14; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: projDelegate.hovered ? Theme.surface2 : "transparent" }
                onClicked: {
                    dlg.projectPicked(dlg._itemId, projDelegate.pid)
                    dlg.close()
                }
            }
        }
    }
}

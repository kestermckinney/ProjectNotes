// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Global search across the whole database (database_search view).
Item {
    id: page
    // Emitted when a result is clicked: datatype + the record id + parent id.
    signal resultActivated(string dataType, string dataId, string fkId)
    // Routed to Main.exportRecord when a result row's menu exports XML.
    signal exportRequested(string table, string id)

    // Current right-click / kebab context (the search model is heterogeneous, so
    // the acted-on row is captured here when the menu opens).
    property string _ctxType: ""
    property string _ctxId: ""
    property string _ctxFk: ""

    function _icon(t) {
        switch (t) {
        case "Project": return "description"
        case "People": return "person"
        case "Client": return "apartment"
        case "Item Tracker": return "task_alt"
        case "Tracker Update": return "forum"
        case "Project Notes": return "edit_note"
        case "Meeting Attendees": return "groups"
        case "Project Locations": return "folder"
        case "Project Team": return "group"
        case "Status Report Item": return "flag"
        }
        return "search"
    }

    // Map a database_search datatype label to the SQL table plugins target
    // (mirrors databaseviews.cpp). Empty ⇒ no plugin/export actions for the row.
    function _table(t) {
        switch (t) {
        case "Project": return "projects"
        case "People": return "people"
        case "Client": return "clients"
        case "Item Tracker": return "item_tracker"
        case "Tracker Update": return "item_tracker_updates"
        case "Project Notes": return "project_notes"
        case "Meeting Attendees": return "meeting_attendees"
        case "Project Locations": return "project_locations"
        case "Project Team": return "project_people"
        case "Status Report Item": return "status_report_items"
        }
        return ""
    }

    // Coalesces bursts of keystrokes into a single performSearch() call so the
    // whole-database query (a big UNION across every table) doesn't re-run on
    // every keypress — same 250ms debounce used for quick-search filtering.
    Timer {
        id: searchDebounce
        interval: 250
        onTriggered: DesktopAppController.performSearch(searchField.text)
    }

    // Configure the shared menu for a result row and open it at scene coords.
    function _openMenu(datatype, dataid, fkId, label, sx, sy) {
        page._ctxType = datatype
        page._ctxId = dataid
        page._ctxFk = fkId
        ctxMenu.recordTable = page._table(datatype)
        ctxMenu.recordType = datatype
        ctxMenu.recordLabel = label
        ctxMenu.openAt(sx, sy)
    }

    // Shared record/plugin menu for search hits. Search rows open a record and
    // may export / run plugins; New/Delete/Filter/Refresh don't apply here.
    RecordContextMenu {
        id: ctxMenu
        recordId: page._ctxId
        canNew: false
        canDelete: false
        canFilter: false
        canRefresh: false
        canExport: ctxMenu.recordTable !== ""   // no table ⇒ nothing to export
        onOpenRequested: page.resultActivated(page._ctxType, page._ctxId, page._ctxFk)
        onExportRequested: page.exportRequested(ctxMenu.recordTable, page._ctxId)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        // Search input
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 44
            radius: Theme.radius
            color: Theme.surface
            border.color: searchField.activeFocus ? Theme.accent : Theme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 12
                spacing: 10
                MaterialIcon { name: "search"; size: 20; color: Theme.text3 }
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search the entire database…")
                    placeholderTextColor: Theme.text3
                    color: Theme.text
                    background: null
                    font.pixelSize: 15
                    selectByMouse: true
                    onTextChanged: {
                        if (text === "") {
                            searchDebounce.stop()
                            DesktopAppController.performSearch(text)
                        } else {
                            searchDebounce.restart()
                        }
                    }
                    Component.onCompleted: forceActiveFocus()
                }
                MaterialIcon {
                    name: "close"; size: 18; color: Theme.text3
                    visible: searchField.text !== ""
                    TapHandler { onTapped: searchField.clear() }
                }
            }
        }

        // Results
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: page.width - 40
                spacing: 8

                Repeater {
                    model: DesktopAppController.searchResultsModel
                    delegate: Card {
                        id: rc
                        required property int index
                        required property var model
                        Layout.fillWidth: true
                        implicitHeight: 60
                        color: rh.hovered ? Theme.raise : Theme.surface

                        function _menu(sx, sy) {
                            page._openMenu(
                                (rc.model.datatype || "").toString(),
                                (rc.model.dataid || "").toString(),
                                (rc.model.fk_id || "").toString(),
                                (rc.model.datadescription || rc.model.dataname || "").toString(),
                                sx, sy)
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 12
                            Rectangle {
                                width: 32; height: 32; radius: 8
                                color: Theme.accentSoft
                                MaterialIcon {
                                    anchors.centerIn: parent
                                    name: page._icon((rc.model.datatype || "").toString())
                                    size: 17; color: Theme.accent
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                RowLayout {
                                    spacing: 8
                                    Text {
                                        text: (rc.model.datatype || "").toString()
                                        color: Theme.text3; font.pixelSize: 10; font.weight: Font.Bold
                                    }
                                    Text {
                                        text: (rc.model.datadescription || rc.model.dataname || "").toString()
                                        color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold
                                        elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                }
                                Text {
                                    text: {
                                        var pn = (rc.model.project_number || "").toString()
                                        var name = (rc.model.project_name || "").toString()
                                        var proj = (pn + " " + name).trim()
                                        var label = (rc.model.dataname || "").toString()
                                        return [label, proj].filter(function(x){ return x.trim() !== "" }).join("  ·  ")
                                    }
                                    color: Theme.text3; font.pixelSize: 11
                                    elide: Text.ElideRight; Layout.fillWidth: true
                                }
                            }
                            KebabButton {
                                implicitWidth: 26; implicitHeight: 26
                                onClicked: (sx, sy) => rc._menu(sx, sy)
                            }
                            MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3 }
                        }
                        HoverHandler { id: rh }
                        TapHandler {
                            onTapped: page.resultActivated(
                                (rc.model.datatype || "").toString(),
                                (rc.model.dataid || "").toString(),
                                (rc.model.fk_id || "").toString())
                        }
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: (ev) => rc._menu(ev.scenePosition.x, ev.scenePosition.y)
                        }
                    }
                }
                Item { Layout.preferredHeight: 8 }
            }
        }
    }
}

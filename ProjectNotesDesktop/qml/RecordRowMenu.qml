// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import ProjectNotesDesktop

// A RecordContextMenu preconfigured for detail-page sub-table rows. Every child
// list on a detail page (tracker items, notes, attendees, …) is its own SQL
// table, so any of them may have plugin menus registered against it. One instance
// per detail page serves all of them: call openFor() with the row's model + id
// and it shows the universally-meaningful built-ins (Export XML · Refresh) plus
// whatever plugin actions target that row's table.
//
// Export can't be run from here (it needs the save-file dialog Main owns), so the
// base menu's exportRequested is re-emitted as exportRecord(table, id) for the
// host page to route up to Main.exportRecord.
RecordContextMenu {
    id: rowMenu

    signal exportRecord(string table, string id)
    // Re-emitted when Duplicate is chosen (only offered when openFor enables it).
    signal duplicateRecord(string table, string id)
    // Re-emitted when Move To… is chosen (only offered when openFor enables it).
    signal moveToRecord(string id)

    // Built-ins that make sense for a child record: the row already carries inline
    // open/add/delete affordances, so the menu adds only Export + Refresh + plugins.
    canOpen: false
    canNew: false
    canDelete: false
    canFilter: false
    canRefresh: true
    canExport: true

    onExportRequested: rowMenu.exportRecord(
        DesktopAppController.tableNameForModel(rowMenu.model), rowMenu.recordId)
    onDuplicateRequested: rowMenu.duplicateRecord(
        DesktopAppController.tableNameForModel(rowMenu.model), rowMenu.recordId)
    onMoveToRequested: rowMenu.moveToRecord(rowMenu.recordId)
    onRefreshRequested: DesktopAppController.refreshModel(rowMenu.model)

    // Configure for one row and open at scene coordinates. Pass allowDuplicate
    // true for row types that support Copy/Duplicate, and allowMoveTo true for
    // row types that support Move To… (both: tracker items only, so far).
    function openFor(rowModel, id, type, label, sx, sy, allowDuplicate, allowMoveTo) {
        rowMenu.model = rowModel
        rowMenu.recordId = id
        rowMenu.recordType = type
        rowMenu.recordLabel = label
        rowMenu.canDuplicate = (allowDuplicate === true)
        rowMenu.canMoveTo = (allowMoveTo === true)
        rowMenu.openAt(sx, sy)
    }
}

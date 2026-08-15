// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// QuickFilterSheet — small action sheet triggered by long-pressing a list
// row, offering pre-filled "filter the list by this row's own value"
// shortcuts. Mirrors the Quick Filter submenu in
// ProjectNotesDesktop/qml/RecordContextMenu.qml, without the rest of that
// menu (Open/Delete/Plugins/etc.) — mobile has no long-press context menu for
// those actions yet, and they're already reachable from the detail page.
// Usage:
//   QuickFilterSheet { id: qfSheet }
//   qfSheet.openWith(AppController.projectsListModel, [
//       { label: qsTr("This Client"), field: "client_id", values: [clientId] }
//   ])

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Popup {
    id: root

    property var _model: null
    property var _filters: []   // [{label, field, values}]

    readonly property bool _hasActive: _model ? AppController.hasActiveColumnFilters(_model) : false

    // ── Public API ────────────────────────────────────────────────────────────
    function openWith(model, filters) {
        _model = model
        _filters = filters || []
        if (_filters.length === 0) return
        open()
    }

    // True if `qf`'s field/values exactly match one of the model's active
    // column filters — drives its checkmark.
    function _matches(qf) {
        var specs = root._model ? AppController.activeColumnFilters(root._model) : []
        for (var i = 0; i < specs.length; i++) {
            if (specs[i].field !== qf.field) continue
            var v = specs[i].values
            if (v.length !== qf.values.length) continue
            var same = true
            for (var j = 0; j < v.length; j++)
                if (v[j] !== qf.values[j]) { same = false; break }
            if (same) return true
        }
        return false
    }

    // ── Popup geometry — slides up from screen bottom, sized to content (the
    // quick-filter list is always short — a handful of shortcuts per row) ────
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    x: 0
    y: parent ? parent.height - height : 0
    width:  parent ? parent.width : 390
    height: parent ? Math.min(_col.implicitHeight, parent.height - 60) : _col.implicitHeight
    padding: 0

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 220; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 180; easing.type: Easing.InCubic }
    }

    background: Rectangle {
        color: palette.base
        radius: 16
        layer.enabled: true
    }

    contentItem: ColumnLayout {
        id: _col
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Label {
                anchors.centerIn: parent
                text: qsTr("Quick Filter")
                font.pixelSize: 17
                font.weight: Font.DemiBold
            }
            ToolButton {
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                icon.name: "xmark"
                focusPolicy: Qt.NoFocus
                onClicked: root.close()
            }
            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: Theme.mutedText; opacity: 0.25
            }
        }

        Repeater {
            model: root._filters
            delegate: ItemDelegate {
                id: rowItem
                required property var modelData
                // Reads filterRev so the checkmark stays live while the sheet
                // is open and the user picks more than one shortcut (like the
                // desktop menu, this stays open across picks — see
                // AppController.filterRev).
                readonly property bool checked_: { AppController.filterRev; return root._matches(modelData) }
                Layout.fillWidth: true
                text: modelData.label

                contentItem: RowLayout {
                    spacing: 10
                    Label { text: rowItem.text; Layout.fillWidth: true; elide: Text.ElideRight }
                    Label { visible: rowItem.checked_; text: "✓"; color: palette.highlight; font.bold: true }
                }
                onClicked: AppController.applyQuickFilter(root._model, modelData.field, modelData.values)
            }
        }

        Rectangle {
            visible: root._hasActive
            Layout.fillWidth: true; height: 1; color: Theme.mutedText; opacity: 0.25
        }

        ItemDelegate {
            visible: root._hasActive
            Layout.fillWidth: true
            text: qsTr("Clear Filters")
            onClicked: {
                AppController.clearColumnFilters(root._model)
                root.close()
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}

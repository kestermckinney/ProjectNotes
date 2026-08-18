// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// QuickFilterDialog — centered modal dialog triggered by long-pressing a list
// row, offering pre-filled "filter the list by this row's own value"
// shortcuts. Mirrors the Quick Filter submenu in
// ProjectNotesDesktop/qml/RecordContextMenu.qml, without the rest of that
// menu (Open/Delete/Plugins/etc.) — mobile has no long-press context menu for
// those actions yet, and they're already reachable from the detail page.
// Usage:
//   QuickFilterDialog { id: qfDialog }
//   qfDialog.openWith(AppController.projectsListModel, [
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

    // Reads filterRev so the trailing "Clear Filters" row appears/disappears
    // as shortcuts are picked while this dialog stays open —
    // hasActiveColumnFilters() has no change notification of its own (see
    // AppController.filterRev), so without this the row's visibility would be
    // frozen at whatever was active when the dialog opened.
    readonly property bool _hasActive: {
        AppController.filterRev
        return _model ? AppController.hasActiveColumnFilters(_model) : false
    }

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

    // ── Geometry — a card centered on the window ─────────────────────────────
    // Parented to the window overlay rather than to the page that declares it:
    // that centers it on the screen instead of within the list, and it keeps
    // the dialog alive if anything hides the page underneath — a popup whose
    // visual parent goes invisible is hidden along with it.
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    width: Math.min(340, (parent ? parent.width : 390) - 48)

    // padding: 0 on its own is not enough: the iOS style sets topPadding (23)
    // as well as padding, and a whole-control padding never overrides a
    // per-edge one — the 23px survives as a blank strip above the header.
    padding: 0
    topPadding: 0

    // Cap on the shortcut list alone (the title bar and Clear Filters row sit
    // outside it), so a long set scrolls instead of running off-screen.
    readonly property real _maxRowsHeight: (parent ? parent.height : 600) * 0.55

    // Outside-tap dismissal is the backdrop TapHandler below rather than
    // Popup.CloseOnPressOutside, because this dialog opens while the finger
    // that long-pressed the row is still down — and that finger's release
    // lands outside the dialog, on the backdrop. With CloseOnPressOutside the
    // tail end of the very gesture that opened the dialog counts as a
    // dismissal, so it vanished the instant the user lifted their finger (or
    // dragged off the row and let go). A TapHandler only fires for a press and
    // release it saw both halves of, so the stray release is ignored while a
    // genuine tap outside still closes.
    closePolicy: Popup.CloseOnEscape
    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.45)
        TapHandler { onTapped: root.close() }
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale";   from: 0.92; to: 1; duration: 160; easing.type: Easing.OutCubic }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 130; easing.type: Easing.InCubic }
            NumberAnimation { property: "scale";   from: 1; to: 0.96; duration: 130; easing.type: Easing.InCubic }
        }
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

        ListView {
            id: rowsList
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, root._maxRowsHeight)
            model: root._filters
            interactive: contentHeight > height
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: ItemDelegate {
                id: rowItem
                required property var modelData
                // Reads filterRev so the checkmark stays live while the dialog
                // is open and the user picks more than one shortcut (this
                // stays open across picks, like the desktop menu — see
                // AppController.filterRev).
                readonly property bool checked_: { AppController.filterRev; return root._matches(modelData) }
                width: ListView.view ? ListView.view.width : 0
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
            Layout.fillWidth: true; Layout.preferredHeight: 1
            color: Theme.mutedText; opacity: 0.25
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

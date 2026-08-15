// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// SortSheet — mobile Sort menu, mirroring ProjectNotesDesktop/qml/SortMenu.qml.
// A column list (scrollable, since some sections have many sortable columns)
// plus an Ascending/Descending toggle and Clear Sort, as a bottom sheet.
// Picking a column or a direction takes effect immediately (like the desktop
// menu) — there's no separate Apply step.
// Usage:
//   SortSheet { id: sortSheet }
//   sortSheet.openFor("projects", qsTr("Projects"))

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Popup {
    id: root

    property var    _model: null
    property string _sectionLabel: ""
    property var    _cols: []          // [{field,label}]
    property string _field: ""
    property bool   _descending: false

    // ── Public API ────────────────────────────────────────────────────────────
    function openFor(section, sectionLabel) {
        _sectionLabel = sectionLabel
        _model = AppController.modelForSection(section)
        if (!_model) return
        _cols = AppController.sortColumns(_model)
        var active = AppController.activeSort(_model)
        _field = active.field || ""
        _descending = active.descending === true
        open()
    }

    function _pick(field) {
        root._field = field
        AppController.applySort(root._model, field, root._descending)
    }
    function _setDirection(desc) {
        root._descending = desc
        if (root._field !== "")
            AppController.applySort(root._model, root._field, desc)
    }
    function _clearSort() {
        AppController.clearSort(root._model)
        root._field = ""
        root._descending = false
        root.close()
    }

    // ── Popup geometry — slides up from screen bottom ─────────────────────────
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    x: 0
    y: parent ? parent.height - height : 0
    width:  parent ? parent.width : 390
    height: parent ? Math.min(parent.height - 60, 560) : 500
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

    // Direction chip shared by the Ascending/Descending row below.
    component DirectionChip: ItemDelegate {
        id: chip
        property bool active: false
        Layout.fillWidth: true
        Layout.preferredHeight: 44
        contentItem: Label {
            text: chip.text
            font.weight: chip.active ? Font.DemiBold : Font.Normal
            color: chip.active ? palette.highlight : palette.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 8
            color: chip.active ? Qt.rgba(palette.highlight.r, palette.highlight.g, palette.highlight.b, 0.16) : "transparent"
            border.color: chip.active ? palette.highlight : palette.mid
            border.width: 1
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ── Header ────────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Label {
                anchors.centerIn: parent
                text: qsTr("Sort") + " · " + root._sectionLabel
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

        // ── Sortable column list (scrollable) ───────────────────────────────────
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root._cols

            header: Label {
                width: ListView.view.width
                text: qsTr("SORT BY")
                font.pixelSize: 11
                font.bold: true
                color: Theme.mutedText
                leftPadding: 16
                topPadding: 8
                bottomPadding: 4
            }

            delegate: ItemDelegate {
                required property var modelData
                width: ListView.view.width
                text: modelData.label

                contentItem: RowLayout {
                    spacing: 10
                    Label {
                        text: modelData.label
                        font.weight: modelData.field === root._field ? Font.DemiBold : Font.Normal
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Label {
                        visible: modelData.field === root._field
                        text: "✓"
                        color: palette.highlight
                        font.bold: true
                    }
                }
                onClicked: root._pick(modelData.field)
            }

            ScrollIndicator.vertical: ScrollIndicator {}
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.mutedText; opacity: 0.25 }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 10
            DirectionChip { text: "↑ " + qsTr("Ascending");  active: !root._descending; onClicked: root._setDirection(false) }
            DirectionChip { text: "↓ " + qsTr("Descending"); active: root._descending;   onClicked: root._setDirection(true) }
        }

        ItemDelegate {
            Layout.fillWidth: true
            text: qsTr("Clear Sort")
            onClicked: root._clearSort()
        }

        Item { Layout.preferredHeight: 8 }
    }
}

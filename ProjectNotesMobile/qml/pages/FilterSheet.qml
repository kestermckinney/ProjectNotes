// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// FilterSheet — mobile Filter Editor, a bottom sheet mirroring
// ProjectNotesDesktop/qml/FilterDialog.qml (column list + per-column values),
// collapsed onto phone width as drill-down navigation (column list -> values)
// instead of desktop's side-by-side panels. Selections only take effect on
// Apply; Cancel/dismiss discards them — same as the desktop dialog.
// Usage:
//   FilterSheet { id: filterSheet }
//   filterSheet.openFor("projects", qsTr("Projects"))

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Popup {
    id: root

    property var    _model: null
    property string _sectionLabel: ""
    property var    _cols: []            // [{field,label,isDate}]
    property int    _curIndex: -1        // -1 = column list; >=0 = drilled into _cols[_curIndex]
    property var    _values: []          // distinct values for the current column: [{value,label}]
    property var    _sel: ({})           // { field: {values:[], search, start, end} }

    readonly property var    _curCol:    (_curIndex >= 0 && _curIndex < _cols.length) ? _cols[_curIndex] : null
    readonly property string _curField:  _curCol ? _curCol.field : ""
    readonly property bool   _curIsDate: _curCol ? _curCol.isDate === true : false

    // ── Public API ────────────────────────────────────────────────────────────
    function openFor(section, sectionLabel) {
        _sectionLabel = sectionLabel
        _model = AppController.modelForSection(section)
        if (!_model) return
        _cols = AppController.filterColumns(_model)
        // Preload whatever's already active (manual or Quick Filter) so Apply
        // doesn't wipe it out from under the user.
        _sel = _preloadSel()
        _curIndex = -1
        open()
    }

    // ── Internal helpers ──────────────────────────────────────────────────────
    function _preloadSel() {
        var specs = AppController.activeColumnFilters(_model)
        var sel = {}
        for (var i = 0; i < specs.length; i++) {
            var s = specs[i]
            sel[s.field] = { values: s.values || [], search: s.search || "",
                              start: s.rangeStart || "", end: s.rangeEnd || "" }
        }
        return sel
    }

    function _entry(field) {
        return _sel[field] || { values: [], search: "", start: "", end: "" }
    }
    function _commit(field, e) {
        var s = Object.assign({}, _sel)
        s[field] = e
        _sel = s
    }
    function _toggleValue(field, v) {
        var e = JSON.parse(JSON.stringify(_entry(field)))
        var i = e.values.indexOf(v)
        if (i >= 0) e.values.splice(i, 1); else e.values.push(v)
        _commit(field, e)
    }
    function _isChecked(field, v) {
        var e = _sel[field]
        return !!(e && e.values.indexOf(v) >= 0)
    }
    function _count(field) {
        var e = _sel[field]
        return e ? e.values.length : 0
    }
    function _active(field) {
        var e = _sel[field]
        return !!(e && (e.values.length > 0
                        || (e.search && e.search !== "")
                        || (e.start && e.start !== "") || (e.end && e.end !== "")))
    }
    function _setSearch(field, t) { var e = JSON.parse(JSON.stringify(_entry(field))); e.search = t; _commit(field, e) }
    function _setStart(field, t)  { var e = JSON.parse(JSON.stringify(_entry(field))); e.start = t;  _commit(field, e) }
    function _setEnd(field, t)    { var e = JSON.parse(JSON.stringify(_entry(field))); e.end = t;    _commit(field, e) }

    function _openColumn(index) {
        _curIndex = index
        _values = !_curIsDate ? AppController.columnDistinctValues(_model, _curField) : []
        var e = _entry(_curField)
        searchInput.text = e.search || ""
        startInput.text  = e.start  || ""
        endInput.text    = e.end    || ""
    }

    function _buildSpecs() {
        var specs = []
        for (var field in _sel) {
            if (!_active(field)) continue
            var e = _sel[field]
            specs.push({ field: field, values: e.values,
                         search: e.search || "", rangeStart: e.start || "", rangeEnd: e.end || "" })
        }
        return specs
    }
    function _apply() {
        AppController.applyColumnFilters(_model, _buildSpecs())
        root.close()
    }

    // ── Popup geometry — slides up from screen bottom ─────────────────────────
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    x: 0
    y: parent ? parent.height - height : 0
    width:  parent ? parent.width : 390
    height: parent ? Math.min(parent.height - 60, 620) : 560
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
        spacing: 0

        // ── Header ────────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            ToolButton {
                anchors { left: parent.left; leftMargin: 4; verticalCenter: parent.verticalCenter }
                visible: root._curIndex >= 0
                icon.name: "chevron.left"
                focusPolicy: Qt.NoFocus
                onClicked: root._curIndex = -1
            }

            Label {
                anchors.centerIn: parent
                anchors.leftMargin: 60; anchors.rightMargin: 60
                width: parent.width - 120
                horizontalAlignment: Text.AlignHCenter
                text: root._curIndex < 0 ? qsTr("Filter") + " · " + root._sectionLabel
                                         : (root._curCol ? root._curCol.label : "")
                font.pixelSize: 17
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            ToolButton {
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                visible: root._curIndex < 0
                icon.name: "xmark"
                focusPolicy: Qt.NoFocus
                onClicked: root.close()
            }
            ToolButton {
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                visible: root._curIndex >= 0 && root._active(root._curField)
                text: qsTr("Clear")
                focusPolicy: Qt.NoFocus
                onClicked: {
                    var s = Object.assign({}, root._sel)
                    delete s[root._curField]
                    root._sel = s
                    searchInput.text = ""; startInput.text = ""; endInput.text = ""
                }
            }

            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: Theme.mutedText; opacity: 0.25
            }
        }

        // ── Level 0: column list ────────────────────────────────────────────────
        ListView {
            visible: root._curIndex < 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root._cols

            delegate: ItemDelegate {
                required property int index
                required property var modelData
                width: ListView.view.width

                contentItem: RowLayout {
                    spacing: 10
                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        color: root._active(modelData.field) ? palette.highlight : "transparent"
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Label {
                        text: modelData.label
                        font.weight: root._active(modelData.field) ? Font.DemiBold : Font.Normal
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Label {
                        visible: root._count(modelData.field) > 0
                        text: root._count(modelData.field)
                        color: palette.highlight
                        font.pixelSize: 12
                        font.bold: true
                    }
                    Label {
                        text: "›"
                        color: Theme.mutedText
                        font.pixelSize: 16
                    }
                }
                onClicked: root._openColumn(index)
            }

            ScrollIndicator.vertical: ScrollIndicator {}
        }

        // ── Level 1: values for the selected column ────────────────────────────
        ColumnLayout {
            visible: root._curIndex >= 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Date range (date columns only)
            ColumnLayout {
                visible: root._curIsDate
                Layout.fillWidth: true
                Layout.margins: 14
                spacing: 8
                Label { text: qsTr("Range"); font.pixelSize: 12; color: Theme.mutedText }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    TextField {
                        id: startInput
                        Layout.fillWidth: true
                        placeholderText: qsTr("Start value")
                        onTextEdited: root._setStart(root._curField, text)
                    }
                    Label { text: "→" }
                    TextField {
                        id: endInput
                        Layout.fillWidth: true
                        placeholderText: qsTr("End value")
                        onTextEdited: root._setEnd(root._curField, text)
                    }
                }
            }

            // Contains-search (non-date columns only)
            ColumnLayout {
                visible: !root._curIsDate
                Layout.fillWidth: true
                Layout.margins: 14
                Layout.bottomMargin: 4
                spacing: 8
                TextField {
                    id: searchInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("Contains…")
                    onTextEdited: root._setSearch(root._curField, text)
                }
            }

            ListView {
                visible: !root._curIsDate && root._values.length > 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root._values

                delegate: ItemDelegate {
                    required property var modelData
                    width: ListView.view.width

                    contentItem: RowLayout {
                        spacing: 10
                        Rectangle {
                            width: 20; height: 20; radius: 5
                            Layout.alignment: Qt.AlignVCenter
                            color: root._isChecked(root._curField, modelData.value) ? palette.highlight : "transparent"
                            border.color: root._isChecked(root._curField, modelData.value) ? palette.highlight : palette.mid
                            border.width: 1
                            Label {
                                anchors.centerIn: parent
                                visible: root._isChecked(root._curField, modelData.value)
                                text: "✓"
                                color: "white"
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                        Label {
                            text: modelData.label
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }
                    onClicked: root._toggleValue(root._curField, modelData.value)
                }

                ScrollIndicator.vertical: ScrollIndicator {}
            }

            Label {
                visible: !root._curIsDate && root._values.length === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: qsTr("No values to list — use Contains above.")
                color: Theme.mutedText
                font.pixelSize: 13
            }

            Item { visible: root._curIsDate; Layout.fillHeight: true }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.mutedText; opacity: 0.25 }

        // ── Footer ───────────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 10

            Button {
                text: qsTr("Reset All")
                flat: true
                onClicked: root._sel = ({})
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Apply")
                highlighted: true
                onClicked: root._apply()
            }
        }
    }
}

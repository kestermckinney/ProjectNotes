// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Filter editor modal — a QML port of the Widgets "Filter Data" dialog, styled to
// match the UI mockup: a column list on the left (active dots + selection counts)
// and a values panel on the right (multi-select checkboxes, contains-search, and a
// date range). Reset all / Clear column / Cancel / Apply along the bottom.
Popup {
    id: dlg

    modal: true
    dim: true
    padding: 0
    parent: Overlay.overlay
    scale: Theme.uiScale   // match the zoomed workspace (centered origin)
    width: 600
    height: Math.min(480, parent ? parent.height - 60 : 480)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    background: Rectangle {
        radius: Theme.radiusLg
        color: Theme.bg
        border.color: Theme.border

        // Swallow presses at the popup layer so they don't leak through to the
        // TapHandlers on the list cards/rows behind this modal. Pointer handlers
        // only take a *passive* grab on press, so without an item here that
        // actually accepts the event, delivery keeps walking front-to-back and
        // also "taps" whatever record sits under the dialog — navigating away as
        // soon as the dialog closes. The interactive controls above this
        // (buttons, checkboxes, inputs) still work via their own passive grabs.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }
    }

    // ── State ─────────────────────────────────────────────────────────────────
    property var    _model: null
    property string _section: ""
    property string _sectionLabel: ""
    property var    _cols: []            // [{field,label,isDate}]
    property int    _curIndex: 0
    property var    _values: []          // distinct values for the current column: [{value,label}]
    property var    _sel: ({})           // { field: {values:[], search, start, end} }

    readonly property var  _curCol: (_cols.length > 0 && _curIndex >= 0 && _curIndex < _cols.length)
                                     ? _cols[_curIndex] : null
    readonly property string _curField: _curCol ? _curCol.field : ""
    readonly property bool _curIsDate: _curCol ? _curCol.isDate === true : false

    function openFor(section) {
        _section = section
        // Model resolution is the one canonical section→model map (also used
        // by Quick Filter/Sort and the top bar's quick-search resync) — this
        // switch now only picks the translated label for the header.
        _model = DesktopAppController.modelForSection(section)
        if (!_model) return
        switch (section) {
        case "projects": _sectionLabel = qsTr("Projects"); break
        case "items":    _sectionLabel = qsTr("Master Items"); break
        case "people":   _sectionLabel = qsTr("People"); break
        case "clients":  _sectionLabel = qsTr("Clients"); break
        case "statusreport": _sectionLabel = qsTr("Status Report Items"); break
        case "trackeritems": _sectionLabel = qsTr("Tracker Items"); break
        case "team":          _sectionLabel = qsTr("Team"); break
        case "locations":     _sectionLabel = qsTr("Locations"); break
        case "notes":         _sectionLabel = qsTr("Notes"); break
        default: _sectionLabel = ""; break
        }
        _cols = DesktopAppController.filterColumns(_model)
        // Preload whatever's already active (e.g. from a Quick Filter, or a
        // filter left over from a previous session) so Apply doesn't wipe it
        // out from under the user — this dialog used to always start blank.
        _sel = _preloadSel()
        _curIndex = 0
        _reloadValues()
        open()
    }

    function _preloadSel() {
        var specs = DesktopAppController.activeColumnFilters(_model)
        var sel = {}
        for (var i = 0; i < specs.length; i++) {
            var s = specs[i]
            sel[s.field] = { values: s.values || [], search: s.search || "",
                              start: s.rangeStart || "", end: s.rangeEnd || "" }
        }
        return sel
    }

    function _reloadValues() {
        _values = _curField !== "" ? DesktopAppController.columnDistinctValues(_model, _curField) : []
        _syncInputs()
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

    // Push the stored per-column text into the input fields when the column changes.
    function _syncInputs() {
        var e = _entry(_curField)
        searchInput.text = e.search || ""
        startInput.text  = e.start || ""
        endInput.text    = e.end || ""
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
        DesktopAppController.applyColumnFilters(_model, _buildSpecs())
        _dismiss()
    }

    // Close on the next tick rather than synchronously inside the tap handler.
    // Closing mid-delivery tears down this modal (and its press-absorbing
    // background) before the press/release has finished propagating, so the tap
    // falls through to the list card behind and navigates. Deferring lets the
    // event finish against the still-present modal, then the popup closes.
    function _dismiss() { Qt.callLater(close) }

    // ── Layout ────────────────────────────────────────────────────────────────
    contentItem: ColumnLayout {
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 8
            MaterialIcon { name: "filter_list"; size: 17; color: Theme.accent; Layout.alignment: Qt.AlignVCenter }
            Text { text: qsTr("Filter Editor"); color: Theme.text; font.pixelSize: 14; font.weight: Font.Bold; verticalAlignment: Text.AlignVCenter }
            Text { text: "· " + dlg._sectionLabel; color: Theme.text3; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter }
            Item { Layout.fillWidth: true }
            Rectangle {
                implicitWidth: 24; implicitHeight: 24; radius: Theme.radiusSm
                color: closeHover.hovered ? Theme.surface2 : "transparent"
                MaterialIcon { anchors.centerIn: parent; name: "close"; size: 16; color: Theme.text2 }
                HoverHandler { id: closeHover }
                // Exclusive grab: a plain TapHandler only takes a passive grab, so
                // without this the same tap also falls through to the list card
                // behind the modal and navigates (matches the Main.qml dialog fix).
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dlg._dismiss() }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        // Body: column list | values panel
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Column list
            Rectangle {
                Layout.preferredWidth: 180
                Layout.fillHeight: true
                color: Theme.sidebar
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    Text {
                        text: qsTr("COLUMN NAME"); color: Theme.text3
                        font.pixelSize: 9; font.weight: Font.Bold
                        Layout.leftMargin: 11; Layout.topMargin: 9; Layout.bottomMargin: 3
                    }
                    ListView {
                        id: colList
                        Layout.fillWidth: true; Layout.fillHeight: true
                        clip: true
                        model: dlg._cols
                        currentIndex: dlg._curIndex
                        delegate: Rectangle {
                            required property int index
                            required property var modelData
                            width: colList.width
                            height: 30
                            color: index === dlg._curIndex ? Theme.accentSoft
                                   : (colHover.hovered ? Theme.surface2 : "transparent")
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 8
                                spacing: 7
                                Rectangle {
                                    implicitWidth: 5; implicitHeight: 5; radius: 2.5
                                    color: dlg._active(modelData.field) ? Theme.accent : "transparent"
                                    Layout.alignment: Qt.AlignVCenter
                                }
                                Text {
                                    text: modelData.label
                                    color: index === dlg._curIndex ? Theme.accent : Theme.text
                                    font.pixelSize: 12
                                    font.weight: (index === dlg._curIndex || dlg._active(modelData.field)) ? Font.DemiBold : Font.Normal
                                    Layout.fillWidth: true; elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Rectangle {
                                    visible: dlg._count(modelData.field) > 0
                                    radius: 8; color: Theme.accentSoft
                                    implicitHeight: 14; implicitWidth: Math.max(16, cLbl.implicitWidth + 8)
                                    Layout.alignment: Qt.AlignVCenter
                                    Text {
                                        id: cLbl; anchors.centerIn: parent
                                        text: dlg._count(modelData.field).toString()
                                        color: Theme.accent; font.pixelSize: 9; font.weight: Font.Bold
                                    }
                                }
                            }
                            HoverHandler { id: colHover }
                            TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: { dlg._curIndex = index; dlg._reloadValues() } }
                        }
                    }
                }
            }
            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.border }

            // Values panel
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.topMargin: 9; Layout.bottomMargin: 3
                    spacing: 6
                    Text { text: qsTr("FILTER VALUES"); color: Theme.text3; font.pixelSize: 9; font.weight: Font.Bold }
                    Text { text: dlg._curCol ? dlg._curCol.label : ""; color: Theme.text3; font.pixelSize: 10 }
                    Item { Layout.fillWidth: true }
                    Text { text: dlg._count(dlg._curField) + qsTr(" selected"); color: Theme.text3; font.pixelSize: 10 }
                }

                // Distinct-value checkboxes (non-date columns)
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.leftMargin: 11; Layout.rightMargin: 11
                    radius: Theme.radiusSm
                    color: Theme.surface
                    border.color: Theme.border
                    clip: true
                    ListView {
                        id: valList
                        anchors.fill: parent
                        visible: !dlg._curIsDate && dlg._values.length > 0
                        clip: true
                        model: dlg._values
                        delegate: Rectangle {
                            required property int index
                            required property var modelData
                            width: valList.width
                            height: 28
                            color: vHover.hovered ? Theme.surface2 : "transparent"
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                spacing: 8
                                Rectangle {
                                    implicitWidth: 14; implicitHeight: 14; radius: 4
                                    Layout.alignment: Qt.AlignVCenter
                                    color: dlg._isChecked(dlg._curField, modelData.value) ? Theme.accent : "transparent"
                                    border.color: dlg._isChecked(dlg._curField, modelData.value) ? Theme.accent : Theme.border
                                    border.width: 1
                                    MaterialIcon {
                                        anchors.centerIn: parent
                                        visible: dlg._isChecked(dlg._curField, modelData.value)
                                        name: "check"; size: 11; color: "#ffffff"
                                    }
                                }
                                Text {
                                    text: modelData.label; color: Theme.text; font.pixelSize: 12
                                    Layout.fillWidth: true; elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            HoverHandler { id: vHover }
                            TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dlg._toggleValue(dlg._curField, modelData.value) }
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 30
                        visible: !dlg._curIsDate && dlg._values.length === 0
                        text: qsTr("No distinct values to list — use the search box below.")
                        color: Theme.text3; font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    }
                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 30
                        visible: dlg._curIsDate
                        text: qsTr("Use the range below to filter by date.")
                        color: Theme.text3; font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    }
                }

                // Contains-search (disabled for date columns)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.topMargin: 8
                    spacing: 4
                    opacity: dlg._curIsDate ? 0.45 : 1.0
                    Text { text: qsTr("Search Text"); color: Theme.text2; font.pixelSize: 11; font.weight: Font.DemiBold }
                    FilterInput {
                        id: searchInput
                        enabled: !dlg._curIsDate
                        placeholder: dlg._curIsDate ? qsTr("(not applicable for dates)") : qsTr("Contains…")
                        onEdited: (t) => dlg._setSearch(dlg._curField, t)
                    }
                }

                // Date range (enabled for date columns)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.topMargin: 6; Layout.bottomMargin: 11
                    spacing: 4
                    opacity: dlg._curIsDate ? 1.0 : 0.45
                    Text { text: qsTr("Range"); color: Theme.text2; font.pixelSize: 11; font.weight: Font.DemiBold }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        FilterInput {
                            id: startInput
                            enabled: dlg._curIsDate
                            Layout.fillWidth: true
                            placeholder: qsTr("Start value")
                            onEdited: (t) => dlg._setStart(dlg._curField, t)
                        }
                        MaterialIcon { name: "arrow_forward"; size: 15; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                        FilterInput {
                            id: endInput
                            enabled: dlg._curIsDate
                            Layout.fillWidth: true
                            placeholder: qsTr("End value")
                            onEdited: (t) => dlg._setEnd(dlg._curField, t)
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        // Footer
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 6
            FooterButton {
                icon: "restart_alt"; label: qsTr("Reset all")
                onClicked: { dlg._sel = ({}); dlg._syncInputs() }
            }
            FooterButton {
                label: qsTr("Clear column")
                onClicked: {
                    var s = Object.assign({}, dlg._sel); delete s[dlg._curField]; dlg._sel = s; dlg._syncInputs()
                }
            }
            Item { Layout.fillWidth: true }
            FooterButton { label: qsTr("Cancel"); onClicked: dlg._dismiss() }
            FooterButton { label: qsTr("Apply"); primary: true; onClicked: dlg._apply() }
        }
    }

    // ── Small building blocks ──────────────────────────────────────────────────
    component FilterInput: Rectangle {
        id: fi
        property alias text: tf.text
        property string placeholder: ""
        signal edited(string text)
        // `enabled` (inherited from Item) cascades to the child TextField.
        Layout.fillWidth: true
        implicitHeight: 28
        implicitWidth: 90
        radius: Theme.radiusSm
        color: Theme.surface
        border.color: tf.activeFocus ? Theme.accent : Theme.border
        TextField {
            id: tf
            anchors.fill: parent
            anchors.leftMargin: 9; anchors.rightMargin: 9
            verticalAlignment: Text.AlignVCenter
            color: Theme.text
            placeholderText: fi.placeholder
            placeholderTextColor: Theme.text3
            background: null
            font.pixelSize: 12
            selectByMouse: true
            onTextEdited: fi.edited(text)
        }
    }

    component FooterButton: Rectangle {
        id: fb
        property string icon: ""
        property string label: ""
        property bool primary: false
        signal clicked()
        implicitHeight: 28
        implicitWidth: fbRow.implicitWidth + 20
        radius: Theme.radiusSm
        color: primary ? (fbHover.hovered ? Theme.accentStrong : Theme.accent)
                       : (fbHover.hovered ? Theme.surface2 : Theme.surface)
        border.color: primary ? "transparent" : Theme.border
        border.width: primary ? 0 : 1
        RowLayout {
            id: fbRow
            anchors.centerIn: parent
            spacing: 5
            MaterialIcon {
                visible: fb.icon !== ""
                name: fb.icon; size: 14
                color: fb.primary ? "#ffffff" : Theme.text2
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: fb.label
                color: fb.primary ? "#ffffff" : Theme.text
                font.pixelSize: 12; font.weight: fb.primary ? Font.DemiBold : Font.Medium
                verticalAlignment: Text.AlignVCenter
            }
        }
        HoverHandler { id: fbHover }
        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: fb.clicked() }
    }
}

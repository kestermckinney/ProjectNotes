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
    width: 680
    height: Math.min(560, parent ? parent.height - 60 : 560)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    background: Rectangle {
        radius: Theme.radiusLg
        color: Theme.bg
        border.color: Theme.border
    }

    // ── State ─────────────────────────────────────────────────────────────────
    property var    _model: null
    property string _section: ""
    property string _sectionLabel: ""
    property var    _cols: []            // [{field,label,isDate}]
    property int    _curIndex: 0
    property var    _values: []          // distinct values for the current column
    property var    _sel: ({})           // { field: {values:[], search, start, end} }

    readonly property var  _curCol: (_cols.length > 0 && _curIndex >= 0 && _curIndex < _cols.length)
                                     ? _cols[_curIndex] : null
    readonly property string _curField: _curCol ? _curCol.field : ""
    readonly property bool _curIsDate: _curCol ? _curCol.isDate === true : false

    function openFor(section) {
        _section = section
        switch (section) {
        case "projects": _model = DesktopAppController.projectsListModel; _sectionLabel = qsTr("Projects"); break
        case "items":    _model = DesktopAppController.allItemsModel;     _sectionLabel = qsTr("Master Items"); break
        case "people":   _model = DesktopAppController.peopleModel;       _sectionLabel = qsTr("People"); break
        case "clients":  _model = DesktopAppController.clientsModel;      _sectionLabel = qsTr("Clients"); break
        default: return
        }
        _cols = DesktopAppController.filterColumns(_model)
        _sel = ({})
        _curIndex = 0
        _reloadValues()
        open()
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
        close()
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    contentItem: ColumnLayout {
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 15
            spacing: 10
            MaterialIcon { name: "filter_list"; size: 20; color: Theme.accent; Layout.alignment: Qt.AlignVCenter }
            Text { text: qsTr("Filter Editor"); color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold; verticalAlignment: Text.AlignVCenter }
            Text { text: "· " + dlg._sectionLabel; color: Theme.text3; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter }
            Item { Layout.fillWidth: true }
            Rectangle {
                implicitWidth: 28; implicitHeight: 28; radius: Theme.radiusSm
                color: closeHover.hovered ? Theme.surface2 : "transparent"
                MaterialIcon { anchors.centerIn: parent; name: "close"; size: 19; color: Theme.text2 }
                HoverHandler { id: closeHover }
                TapHandler { onTapped: dlg.close() }
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
                Layout.preferredWidth: 210
                Layout.fillHeight: true
                color: Theme.sidebar
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    Text {
                        text: qsTr("COLUMN NAME"); color: Theme.text3
                        font.pixelSize: 10; font.weight: Font.Bold
                        Layout.leftMargin: 14; Layout.topMargin: 11; Layout.bottomMargin: 4
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
                            height: 34
                            color: index === dlg._curIndex ? Theme.accentSoft
                                   : (colHover.hovered ? Theme.surface2 : "transparent")
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 10
                                spacing: 9
                                Rectangle {
                                    implicitWidth: 6; implicitHeight: 6; radius: 3
                                    color: dlg._active(modelData.field) ? Theme.accent : "transparent"
                                    Layout.alignment: Qt.AlignVCenter
                                }
                                Text {
                                    text: modelData.label
                                    color: index === dlg._curIndex ? Theme.accent : Theme.text
                                    font.pixelSize: 13
                                    font.weight: (index === dlg._curIndex || dlg._active(modelData.field)) ? Font.DemiBold : Font.Normal
                                    Layout.fillWidth: true; elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Rectangle {
                                    visible: dlg._count(modelData.field) > 0
                                    radius: 9; color: Theme.accentSoft
                                    implicitHeight: 16; implicitWidth: Math.max(18, cLbl.implicitWidth + 10)
                                    Layout.alignment: Qt.AlignVCenter
                                    Text {
                                        id: cLbl; anchors.centerIn: parent
                                        text: dlg._count(modelData.field).toString()
                                        color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
                                    }
                                }
                            }
                            HoverHandler { id: colHover }
                            TapHandler { onTapped: { dlg._curIndex = index; dlg._reloadValues() } }
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
                    Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.topMargin: 12; Layout.bottomMargin: 4
                    spacing: 8
                    Text { text: qsTr("FILTER VALUES"); color: Theme.text3; font.pixelSize: 10; font.weight: Font.Bold }
                    Text { text: dlg._curCol ? dlg._curCol.label : ""; color: Theme.text3; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    Text { text: dlg._count(dlg._curField) + qsTr(" selected"); color: Theme.text3; font.pixelSize: 11 }
                }

                // Distinct-value checkboxes (non-date columns)
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.leftMargin: 14; Layout.rightMargin: 14
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
                            height: 32
                            color: vHover.hovered ? Theme.surface2 : "transparent"
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 12
                                spacing: 10
                                Rectangle {
                                    implicitWidth: 16; implicitHeight: 16; radius: 4
                                    Layout.alignment: Qt.AlignVCenter
                                    color: dlg._isChecked(dlg._curField, modelData) ? Theme.accent : "transparent"
                                    border.color: dlg._isChecked(dlg._curField, modelData) ? Theme.accent : Theme.border
                                    border.width: 1
                                    MaterialIcon {
                                        anchors.centerIn: parent
                                        visible: dlg._isChecked(dlg._curField, modelData)
                                        name: "check"; size: 13; color: "#ffffff"
                                    }
                                }
                                Text {
                                    text: modelData; color: Theme.text; font.pixelSize: 13
                                    Layout.fillWidth: true; elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            HoverHandler { id: vHover }
                            TapHandler { onTapped: dlg._toggleValue(dlg._curField, modelData) }
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 36
                        visible: !dlg._curIsDate && dlg._values.length === 0
                        text: qsTr("No distinct values to list — use the search box below.")
                        color: Theme.text3; font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    }
                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 36
                        visible: dlg._curIsDate
                        text: qsTr("Use the range below to filter by date.")
                        color: Theme.text3; font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    }
                }

                // Contains-search (disabled for date columns)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.topMargin: 10
                    spacing: 5
                    opacity: dlg._curIsDate ? 0.45 : 1.0
                    Text { text: qsTr("Search Text"); color: Theme.text2; font.pixelSize: 12; font.weight: Font.DemiBold }
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
                    Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.topMargin: 8; Layout.bottomMargin: 14
                    spacing: 5
                    opacity: dlg._curIsDate ? 1.0 : 0.45
                    Text { text: qsTr("Range"); color: Theme.text2; font.pixelSize: 12; font.weight: Font.DemiBold }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        FilterInput {
                            id: startInput
                            enabled: dlg._curIsDate
                            Layout.fillWidth: true
                            placeholder: qsTr("Start value")
                            onEdited: (t) => dlg._setStart(dlg._curField, t)
                        }
                        MaterialIcon { name: "arrow_forward"; size: 18; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
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
            Layout.margins: 12
            spacing: 8
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
            FooterButton { label: qsTr("Cancel"); onClicked: dlg.close() }
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
        implicitHeight: 32
        implicitWidth: 100
        radius: Theme.radiusSm
        color: Theme.surface
        border.color: tf.activeFocus ? Theme.accent : Theme.border
        TextField {
            id: tf
            anchors.fill: parent
            anchors.leftMargin: 10; anchors.rightMargin: 10
            verticalAlignment: Text.AlignVCenter
            color: Theme.text
            placeholderText: fi.placeholder
            placeholderTextColor: Theme.text3
            background: null
            font.pixelSize: 13
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
        implicitHeight: 34
        implicitWidth: fbRow.implicitWidth + 26
        radius: Theme.radiusSm
        color: primary ? (fbHover.hovered ? Theme.accentStrong : Theme.accent)
                       : (fbHover.hovered ? Theme.surface2 : Theme.surface)
        border.color: primary ? "transparent" : Theme.border
        border.width: primary ? 0 : 1
        RowLayout {
            id: fbRow
            anchors.centerIn: parent
            spacing: 6
            MaterialIcon {
                visible: fb.icon !== ""
                name: fb.icon; size: 17
                color: fb.primary ? "#ffffff" : Theme.text2
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: fb.label
                color: fb.primary ? "#ffffff" : Theme.text
                font.pixelSize: 13; font.weight: fb.primary ? Font.DemiBold : Font.Medium
                verticalAlignment: Text.AlignVCenter
            }
        }
        HoverHandler { id: fbHover }
        TapHandler { onTapped: fb.clicked() }
    }
}

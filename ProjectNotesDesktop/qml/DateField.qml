// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import ProjectNotesDesktop

// Labeled date input. Stores/produces dates as MM/DD/YYYY (the format the data
// models display and accept). Type directly, or click the calendar to pick.
ColumnLayout {
    id: root
    property string label: ""
    property string text: ""          // MM/DD/YYYY
    signal edited(string value)

    // Live contents of the box. `text` only catches up when editing finishes,
    // so callers that have to read or replace what's typed while the box still
    // has focus (the Filter Editor's Start/End boxes — its buttons are
    // TapHandlers that never take focus away) go through these instead.
    property alias editText: field.text
    function setText(value) { text = value; field.text = value }

    spacing: 3
    Layout.fillWidth: true

    function _parse(s) {
        var m = /^(\d{1,2})\/(\d{1,2})\/(\d{4})$/.exec((s || "").trim())
        if (!m) return null
        var d = new Date(parseInt(m[3]), parseInt(m[1]) - 1, parseInt(m[2]))
        return isNaN(d.getTime()) ? null : d
    }
    function _fmt(d) {
        var mo = d.getMonth() + 1, da = d.getDate(), y = d.getFullYear()
        return (mo < 10 ? "0" : "") + mo + "/" + (da < 10 ? "0" : "") + da + "/" + y
    }

    Text {
        text: root.label
        visible: root.label !== ""
        color: Theme.text3
        font.pixelSize: Theme.fontXs
        font.weight: Font.DemiBold
    }

    Rectangle {
        id: fieldBox
        Layout.fillWidth: true
        implicitHeight: 30
        radius: Theme.radiusSm
        color: Theme.surface
        border.color: (field.activeFocus || popup.visible) ? Theme.accent : Theme.border

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 9
            anchors.rightMargin: 5
            spacing: 3
            TextField {
                id: field
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
                text: root.text
                placeholderText: "MM/DD/YYYY"
                placeholderTextColor: Theme.text3
                color: Theme.text
                background: null
                font.pixelSize: Theme.fontBody
                selectByMouse: true
                onEditingFinished: { root.text = text; root.edited(text) }
                // Keep in sync when the bound value changes externally.
                Connections {
                    target: root
                    function onTextChanged() { if (!field.activeFocus) field.text = root.text }
                }
            }
            Rectangle {
                Layout.preferredWidth: 22; Layout.preferredHeight: 22; radius: Theme.radiusSm
                color: calHover.hovered ? Theme.surface2 : "transparent"
                MaterialIcon { anchors.centerIn: parent; name: "calendar_today"; size: 14; color: Theme.text2 }
                HoverHandler { id: calHover }
                TapHandler { onTapped: popup.openAt(root._parse(root.text)) }
            }
        }

        // ── Calendar popup ────────────────────────────────────────────────────
        Popup {
            id: popup
            width: 260
            padding: 10
            modal: false
            // Deliberately an in-scene popup (default Popup.Item) — popupType:
            // Popup.Window (Qt 6.10) forwards clicks on the calendar to the main
            // window underneath instead of picking the day.
            scale: Theme.uiScale
            transformOrigin: Item.TopLeft
            background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

            // Clicking away dismisses the calendar and nothing else — see
            // ClickShield.qml. A non-modal popup like this one blocks nothing
            // at all, so its dismissing click always reached the page behind.
            ClickShield { host: popup }

            property int shownMonth: (new Date()).getMonth()
            property int shownYear: (new Date()).getFullYear()

            function openAt(d) {
                var base = d ? d : new Date()
                shownMonth = base.getMonth()
                shownYear = base.getFullYear()
                open()
            }

            // Position the popup below the field, but flip above / clamp sideways
            // so a field low or near the edge of the window isn't clipped.
            onAboutToShow: _place()
            function _place() {
                var win = fieldBox.Window.window
                var ph = popup.height > 0 ? popup.height : 300
                if (!win) { popup.x = 0; popup.y = fieldBox.height + 4; return }
                var pos = fieldBox.mapToItem(null, 0, 0)
                if (pos.y + fieldBox.height + 4 + ph > win.height && pos.y - ph - 4 >= 0)
                    popup.y = -ph - 4
                else
                    popup.y = fieldBox.height + 4
                var px = 0
                if (pos.x + popup.width > win.width)
                    px = win.width - popup.width - pos.x
                if (pos.x + px < 0)
                    px = -pos.x
                popup.x = px
            }
            function _prev() {
                if (shownMonth === 0) { shownMonth = 11; shownYear-- }
                else shownMonth--
            }
            function _next() {
                if (shownMonth === 11) { shownMonth = 0; shownYear++ }
                else shownMonth++
            }

            readonly property var _monthNames: ["January","February","March","April","May","June",
                                                "July","August","September","October","November","December"]

            contentItem: ColumnLayout {
                spacing: 5
                RowLayout {
                    Layout.fillWidth: true
                    Rectangle {
                        width: 22; height: 22; radius: Theme.radiusSm; color: pHover.hovered ? Theme.surface2 : "transparent"
                        MaterialIcon { anchors.centerIn: parent; name: "chevron_left"; size: 16; color: Theme.text2 }
                        HoverHandler { id: pHover }
                        TapHandler { onTapped: popup._prev() }
                    }
                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: popup._monthNames[popup.shownMonth] + " " + popup.shownYear
                        color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                    }
                    Rectangle {
                        width: 22; height: 22; radius: Theme.radiusSm; color: nHover.hovered ? Theme.surface2 : "transparent"
                        MaterialIcon { anchors.centerIn: parent; name: "chevron_right"; size: 16; color: Theme.text2 }
                        HoverHandler { id: nHover }
                        TapHandler { onTapped: popup._next() }
                    }
                }
                DayOfWeekRow {
                    Layout.fillWidth: true
                    delegate: Text {
                        required property var model
                        horizontalAlignment: Text.AlignHCenter
                        text: model.shortName
                        color: Theme.text3; font.pixelSize: Theme.font2xs; font.weight: Font.Bold
                    }
                }
                MonthGrid {
                    id: grid
                    Layout.fillWidth: true
                    month: popup.shownMonth
                    year: popup.shownYear
                    delegate: Item {
                        required property var model
                        implicitWidth: 28; implicitHeight: 24
                        readonly property bool inMonth: model.month === popup.shownMonth
                        Rectangle {
                            anchors.centerIn: parent
                            width: 22; height: 20; radius: Theme.radiusSm
                            color: dtHover.hovered && inMonth ? Theme.accentSoft : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: model.day
                                color: inMonth ? Theme.text : Theme.text3
                                font.pixelSize: Theme.fontSm
                            }
                            HoverHandler { id: dtHover }
                            TapHandler {
                                onTapped: {
                                    var picked = root._fmt(model.date)
                                    root.text = picked
                                    field.text = picked
                                    root.edited(picked)
                                    popup.close()
                                }
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Button {
                        flat: true
                        contentItem: Text { text: qsTr("Clear"); color: Theme.red; font.pixelSize: Theme.fontBody }
                        background: null
                        onClicked: { root.text = ""; field.text = ""; root.edited(""); popup.close() }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        flat: true
                        contentItem: Text { text: qsTr("Today"); color: Theme.accent; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold }
                        background: null
                        onClicked: {
                            var t = root._fmt(new Date())
                            root.text = t; field.text = t; root.edited(t); popup.close()
                        }
                    }
                }
            }
        }
    }
}

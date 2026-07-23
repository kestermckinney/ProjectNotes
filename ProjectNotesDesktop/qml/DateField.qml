// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Labeled date input. Stores/produces dates as MM/DD/YYYY (the format the data
// models display and accept). Type directly, or click the calendar to pick.
ColumnLayout {
    id: root
    property string label: ""
    property string text: ""          // MM/DD/YYYY
    signal edited(string value)

    spacing: 4
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
        font.pixelSize: 11
        font.weight: Font.DemiBold
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 34
        radius: Theme.radiusSm
        color: Theme.surface
        border.color: (field.activeFocus || popup.visible) ? Theme.accent : Theme.border

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 6
            spacing: 4
            TextField {
                id: field
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
                text: root.text
                placeholderText: "MM/DD/YYYY"
                placeholderTextColor: Theme.text3
                color: Theme.text
                background: null
                font.pixelSize: 13
                selectByMouse: true
                onEditingFinished: { root.text = text; root.edited(text) }
                // Keep in sync when the bound value changes externally.
                Connections {
                    target: root
                    function onTextChanged() { if (!field.activeFocus) field.text = root.text }
                }
            }
            Rectangle {
                Layout.preferredWidth: 26; Layout.preferredHeight: 26; radius: 6
                color: calHover.hovered ? Theme.surface2 : "transparent"
                MaterialIcon { anchors.centerIn: parent; name: "calendar_today"; size: 16; color: Theme.text2 }
                HoverHandler { id: calHover }
                TapHandler { onTapped: popup.openAt(root._parse(root.text)) }
            }
        }

        // ── Calendar popup ────────────────────────────────────────────────────
        Popup {
            id: popup
            y: parent.height + 4
            width: 260
            padding: 10
            modal: false
            background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

            property int shownMonth: (new Date()).getMonth()
            property int shownYear: (new Date()).getFullYear()

            function openAt(d) {
                var base = d ? d : new Date()
                shownMonth = base.getMonth()
                shownYear = base.getFullYear()
                open()
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
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Rectangle {
                        width: 26; height: 26; radius: 6; color: pHover.hovered ? Theme.surface2 : "transparent"
                        MaterialIcon { anchors.centerIn: parent; name: "chevron_left"; size: 18; color: Theme.text2 }
                        HoverHandler { id: pHover }
                        TapHandler { onTapped: popup._prev() }
                    }
                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: popup._monthNames[popup.shownMonth] + " " + popup.shownYear
                        color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold
                    }
                    Rectangle {
                        width: 26; height: 26; radius: 6; color: nHover.hovered ? Theme.surface2 : "transparent"
                        MaterialIcon { anchors.centerIn: parent; name: "chevron_right"; size: 18; color: Theme.text2 }
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
                        color: Theme.text3; font.pixelSize: 10; font.weight: Font.Bold
                    }
                }
                MonthGrid {
                    id: grid
                    Layout.fillWidth: true
                    month: popup.shownMonth
                    year: popup.shownYear
                    delegate: Item {
                        required property var model
                        implicitWidth: 32; implicitHeight: 28
                        readonly property bool inMonth: model.month === popup.shownMonth
                        Rectangle {
                            anchors.centerIn: parent
                            width: 26; height: 24; radius: 6
                            color: dtHover.hovered && inMonth ? Theme.accentSoft : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: model.day
                                color: inMonth ? Theme.text : Theme.text3
                                font.pixelSize: 12
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
                        contentItem: Text { text: qsTr("Clear"); color: Theme.red; font.pixelSize: 12 }
                        background: null
                        onClicked: { root.text = ""; field.text = ""; root.edited(""); popup.close() }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        flat: true
                        contentItem: Text { text: qsTr("Today"); color: Theme.accent; font.pixelSize: 12; font.weight: Font.DemiBold }
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

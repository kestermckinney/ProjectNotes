// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// DatePickerPopup — grid calendar date picker (bottom sheet).
// Usage:
//   DatePickerPopup {
//       id: picker
//       onDateSelected: function(dateStr) { myField.text = dateStr }
//   }
//   picker.openWithDate("04/15/2026")   // or "" for today

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Popup {
    id: root

    signal dateSelected(string dateString)

    property var _selectedDate: null   // null = no date chosen
    property int _displayMonth: 0      // 0-based month shown in grid
    property int _displayYear:  2026

    // ── Public API ────────────────────────────────────────────────────────────
    function openWithDate(dateStr) {
        var d = _parseDate(dateStr)
        if (d) {
            _selectedDate  = d
            _displayMonth  = d.getMonth()
            _displayYear   = d.getFullYear()
        } else {
            _selectedDate = null
            var today = new Date()
            _displayMonth = today.getMonth()
            _displayYear  = today.getFullYear()
        }
        open()
    }

    // ── Internal helpers ──────────────────────────────────────────────────────
    function _parseDate(str) {
        if (str && str.match(/^\d{1,2}\/\d{1,2}\/\d{4}$/)) {
            var p = str.split("/")
            return new Date(parseInt(p[2]), parseInt(p[0]) - 1, parseInt(p[1]))
        }
        if (str && str.match(/^\d{4}-\d{2}-\d{2}$/)) {
            var p2 = str.split("-")
            return new Date(parseInt(p2[0]), parseInt(p2[1]) - 1, parseInt(p2[2]))
        }
        return null
    }

    function _daysInMonth(month, year) {
        return new Date(year, month + 1, 0).getDate()
    }

    function _firstWeekday(month, year) {
        return new Date(year, month, 1).getDay()   // 0 = Sunday
    }

    function _pad2(n) { return n < 10 ? "0" + n : "" + n }

    function _emitSelected(d) {
        root.dateSelected(_pad2(d.getMonth() + 1) + "/" + _pad2(d.getDate()) + "/" + d.getFullYear())
    }

    property string _monthLabel: Qt.locale().monthName(_displayMonth, Locale.LongFormat) + " " + _displayYear

    // ── Popup geometry — slides up from screen bottom ─────────────────────────
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    x: 0
    property real slideY: 0
    y: (parent ? parent.height - height : 0) + slideY
    width:  parent ? parent.width : 390
    height: 420
    padding: 0

    enter: Transition {
        NumberAnimation { target: root; property: "slideY"; from: root.height; to: 0; duration: 280; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { target: root; property: "slideY"; from: 0; to: root.height; duration: 220; easing.type: Easing.InCubic }
    }

    background: Rectangle {
        color: palette.base
        radius: 16
        layer.enabled: true
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ── Header bar: Cancel | Select Date | Done ───────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "transparent"

            RowLayout {
                anchors { fill: parent; leftMargin: 8; rightMargin: 8 }

                Button {
                    text: qsTr("Cancel")
                    flat: true
                    onClicked: root.close()
                }

                Label {
                    text: qsTr("Select Date")
                    font.pixelSize: 16
                    font.bold: true
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Button {
                    text: qsTr("Done")
                    flat: true
                    font.bold: true
                    enabled: root._selectedDate !== null
                    onClicked: {
                        if (root._selectedDate)
                            root._emitSelected(root._selectedDate)
                        root.close()
                    }
                }
            }

            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: palette.placeholderText; opacity: 0.3
            }
        }

        // ── Month navigation: ‹ April 2026 › ─────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.leftMargin: 4
            Layout.rightMargin: 4

            Button {
                text: "‹"
                flat: true
                font.pixelSize: 24
                onClicked: {
                    if (root._displayMonth === 0) { root._displayMonth = 11; root._displayYear-- }
                    else                          { root._displayMonth-- }
                }
            }

            Label {
                Layout.fillWidth: true
                text: root._monthLabel
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 16
                font.bold: true
            }

            Button {
                text: "›"
                flat: true
                font.pixelSize: 24
                onClicked: {
                    if (root._displayMonth === 11) { root._displayMonth = 0; root._displayYear++ }
                    else                           { root._displayMonth++ }
                }
            }
        }

        // ── Day-of-week header row ────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            implicitHeight: 28

            Row {
                anchors.fill: parent
                Repeater {
                    model: ["S","M","T","W","T","F","S"]
                    delegate: Item {
                        width: root.width / 7
                        height: 28
                        Label {
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: 12
                            font.bold: true
                            color: palette.placeholderText
                        }
                    }
                }
            }
        }

        // ── Calendar grid (6 rows × 7 cols = 42 cells) ───────────────────────
        Item {
            id: gridContainer
            Layout.fillWidth: true
            Layout.fillHeight: true

            Grid {
                anchors.fill: parent
                columns: 7

                Repeater {
                    model: 42

                    delegate: Item {
                        id: cell

                        readonly property int _firstWD:  root._firstWeekday(root._displayMonth, root._displayYear)
                        readonly property int _mDays:    root._daysInMonth(root._displayMonth, root._displayYear)
                        readonly property int _prevDays: root._daysInMonth(
                            root._displayMonth > 0 ? root._displayMonth - 1 : 11,
                            root._displayMonth > 0 ? root._displayYear      : root._displayYear - 1)

                        readonly property int _offset: index - _firstWD

                        readonly property bool isCurrent: _offset >= 0 && _offset < _mDays
                        readonly property int  dayNum: {
                            if (_offset < 0)     return _prevDays + _offset + 1
                            if (_offset >= _mDays) return _offset - _mDays + 1
                            return _offset + 1
                        }

                        readonly property bool isToday: {
                            if (!isCurrent) return false
                            var t = new Date()
                            return t.getFullYear() === root._displayYear &&
                                   t.getMonth()    === root._displayMonth &&
                                   t.getDate()     === dayNum
                        }
                        readonly property bool isSelected: {
                            if (!isCurrent || !root._selectedDate) return false
                            return root._selectedDate.getFullYear() === root._displayYear &&
                                   root._selectedDate.getMonth()    === root._displayMonth &&
                                   root._selectedDate.getDate()     === dayNum
                        }

                        width:  gridContainer.width / 7
                        height: gridContainer.height / 6

                        // selected: filled navy circle; today: light tint circle
                        Rectangle {
                            anchors.centerIn: parent
                            width: 34; height: 34; radius: 17
                            color: isSelected ? Theme.navyMid
                                 : isToday    ? Qt.rgba(0.165, 0.249, 0.435, 0.18)
                                              : "transparent"
                        }

                        Label {
                            anchors.centerIn: parent
                            text: dayNum
                            font.pixelSize: 15
                            font.bold: isSelected || isToday
                            color: isSelected   ? "white"
                                 : isToday      ? Theme.navyMid
                                 : isCurrent    ? palette.text
                                                : palette.placeholderText
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: isCurrent
                            onClicked: root._selectedDate = new Date(root._displayYear, root._displayMonth, dayNum)
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: palette.placeholderText; opacity: 0.2 }

        // ── Footer: Today | Clear ─────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            Button {
                text: qsTr("Today")
                flat: true
                onClicked: {
                    var t = new Date()
                    root._selectedDate = t
                    root._displayMonth = t.getMonth()
                    root._displayYear  = t.getFullYear()
                    root._emitSelected(t)
                    root.close()
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Clear")
                flat: true
                onClicked: {
                    root._selectedDate = null
                    root.dateSelected("")
                    root.close()
                }
            }
        }

        // Safe-area spacer
        Item { Layout.fillWidth: true; height: 16 }
    }
}

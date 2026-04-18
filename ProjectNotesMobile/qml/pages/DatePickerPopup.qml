// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// DatePickerPopup — iOS-style bottom-sheet date picker with three tumbler wheels.
// Usage:
//   DatePickerPopup {
//       id: picker
//       onDateSelected: function(dateStr) { myField.text = dateStr }
//   }
//   picker.openWithDate("2026-04-11")   // or "" for today

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    signal dateSelected(string dateString)

    // Holds the parsed date until onOpened fires; set by openWithDate().
    property var _pendingDate: null

    // ── Public API ────────────────────────────────────────────────────────────
    function openWithDate(dateStr) {
        _pendingDate = _parseDate(dateStr)
        open()
    }

    // Apply Tumbler indices after the popup is fully open.
    // Set currentIndex synchronously for logical state, then positionViewAtIndex
    // in Qt.callLater forces the ListView scroll position after the contentItem
    // is accessible, ensuring the wheels land on the correct date.
    onOpened: {
        if (_pendingDate) {
            var d = _pendingDate
            _pendingDate = null

            // Compute year index arithmetically — avoids indexOf() unreliability
            // when the yearModel binding hasn't fully settled.
            var firstYear = parseInt(yearModel[0])
            var yearIdx   = Math.max(0, Math.min(
                                d.getFullYear() - firstYear,
                                yearModel.length - 1))

            yearTumbler.currentIndex  = yearIdx
            monthTumbler.currentIndex = d.getMonth()
            dayTumbler.currentIndex   = d.getDate() - 1

            Qt.callLater(function() {
                yearTumbler.contentItem.positionViewAtIndex(yearTumbler.currentIndex, ListView.Center)
                monthTumbler.contentItem.positionViewAtIndex(monthTumbler.currentIndex, ListView.Center)
                dayTumbler.contentItem.positionViewAtIndex(dayTumbler.currentIndex, ListView.Center)
            })
        }
    }

    // ── Internal helpers ──────────────────────────────────────────────────────
    function _parseDate(str) {
        // Primary format: MM/dd/yyyy (what the database model returns)
        if (str && str.match(/^\d{1,2}\/\d{1,2}\/\d{4}$/)) {
            var p = str.split("/")
            return new Date(parseInt(p[2]), parseInt(p[0]) - 1, parseInt(p[1]))
        }
        // Fallback: YYYY-MM-DD
        if (str && str.match(/^\d{4}-\d{2}-\d{2}$/)) {
            var p2 = str.split("-")
            return new Date(parseInt(p2[0]), parseInt(p2[1]) - 1, parseInt(p2[2]))
        }
        return new Date()
    }

    function _daysInMonth(month, year) {
        return new Date(year, month + 1, 0).getDate()
    }

    function _paddedDay(n) { return n < 10 ? "0" + n : "" + n }

    function _selectedDateString() {
        var year  = yearModel[yearTumbler.currentIndex]
        var month = monthTumbler.currentIndex + 1
        var day   = dayTumbler.currentIndex + 1
        var daysMax = _daysInMonth(monthTumbler.currentIndex, parseInt(year))
        if (day > daysMax) day = daysMax
        return _paddedDay(month) + "/" + _paddedDay(day) + "/" + year
    }

    // ── Year model (20 years centred on today) ────────────────────────────────
    property var yearModel: {
        var arr = []
        var end = new Date().getFullYear() + 14
        for (var y = 1900; y <= end; y++)
            arr.push(y.toString())
        return arr
    }

    // ── Day model (reactive to month + year) ─────────────────────────────────
    property var dayModel: {
        var count = _daysInMonth(
            monthTumbler.currentIndex,
            parseInt(yearModel[yearTumbler.currentIndex] || new Date().getFullYear()))
        var arr = []
        for (var i = 1; i <= count; i++) arr.push(_paddedDay(i))
        return arr
    }

    // ── Popup geometry — slides up from screen bottom ─────────────────────────
    // The declaring item sets parent: Overlay.overlay before opening this popup,
    // so x/y here are in overlay (full-screen) coordinates — bottom positioning works.
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    x: 0
    // slideY is an animated offset so the y binding is never broken by animation.
    // enter slides it from +height (off below) → 0; exit slides 0 → +height.
    property real slideY: 0
    y: (parent ? parent.height - height : 0) + slideY
    width:  parent ? parent.width : 390
    height: 300
    padding: 0

    // Slide-up enter / slide-down exit — animate slideY, NOT y
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

        // ── Header bar ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "transparent"

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }

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
                    onClicked: {
                        root.dateSelected(root._selectedDateString())
                        root.close()
                    }
                }
            }

            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1
                color: palette.placeholderText
                opacity: 0.3
            }
        }

        // ── Tumbler row ───────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Selection highlight band
            Rectangle {
                anchors.centerIn: parent
                width: parent.width
                height: parent.tumblerHeight
                color: palette.button
                opacity: 0.4
                radius: 8
            }

            property int tumblerHeight: 44

            RowLayout {
                anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
                spacing: 0

                // Month
                Tumbler {
                    id: monthTumbler
                    Layout.fillWidth: true
                    wrap: false
                    model: ["Jan","Feb","Mar","Apr","May","Jun",
                            "Jul","Aug","Sep","Oct","Nov","Dec"]
                    visibleItemCount: 5
                    delegate: tumblerDelegate
                    onCurrentIndexChanged: {
                        var maxDay = root._daysInMonth(currentIndex,
                            parseInt(root.yearModel[yearTumbler.currentIndex] || new Date().getFullYear()))
                        if (dayTumbler.currentIndex >= maxDay)
                            dayTumbler.currentIndex = maxDay - 1
                    }
                }

                // Day
                Tumbler {
                    id: dayTumbler
                    Layout.fillWidth: true
                    wrap: false
                    model: root.dayModel
                    visibleItemCount: 5
                    delegate: tumblerDelegate
                }

                // Year
                Tumbler {
                    id: yearTumbler
                    Layout.fillWidth: true
                    wrap: false
                    model: root.yearModel
                    visibleItemCount: 5
                    delegate: tumblerDelegate
                    onCurrentIndexChanged: {
                        var maxDay = root._daysInMonth(monthTumbler.currentIndex,
                            parseInt(root.yearModel[currentIndex] || new Date().getFullYear()))
                        if (dayTumbler.currentIndex >= maxDay)
                            dayTumbler.currentIndex = maxDay - 1
                    }
                }
            }
        }

        // Bottom safe-area spacer
        Item { Layout.fillWidth: true; height: 16 }
    }

    // ── Shared tumbler delegate ───────────────────────────────────────────────
    Component {
        id: tumblerDelegate

        Label {
            text: modelData
            opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
            font.pixelSize: Math.abs(Tumbler.displacement) < 0.5 ? 20 : 16
            font.bold: Math.abs(Tumbler.displacement) < 0.5
            color: Math.abs(Tumbler.displacement) < 0.5 ? palette.text : palette.placeholderText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}

// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Shared date input row: tapping opens the grid calendar picker.
// Usage:
//   DateFieldRow { id: myDate; text: someInitialValue }
//   then read myDate.text when saving.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property string text: ""
    property string placeholderText: qsTr("MM/DD/YYYY")

    // Call before saving: commits a date selected in the popup but not yet confirmed via Done.
    // Handles the case where the user picks a date then navigates back without tapping Done.
    function commitPending() {
        if (picker.opened && picker._selectedDate !== null) {
            var d = picker._selectedDate
            var m  = d.getMonth() + 1
            var dy = d.getDate()
            var y  = d.getFullYear()
            text = (m  < 10 ? "0" + m  : "" + m)  + "/"
                 + (dy < 10 ? "0" + dy : "" + dy) + "/"
                 + y
        }
        if (picker.opened) picker.close()
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 44
    color: palette.base

    // Tap anywhere on the row to open the picker
    MouseArea {
        anchors.fill: parent
        onClicked: picker.openWithDate(root.text)
    }

    RowLayout {
        anchors { fill: parent; leftMargin: 16; rightMargin: 8 }
        spacing: 0

        Label {
            id: dateLabel
            Layout.fillWidth: true
            text: root.text !== "" ? root.text : root.placeholderText
            color: root.text !== "" ? palette.text : palette.placeholderText
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 16
        }

        Button {
            text: qsTr("Clear")
            font.pixelSize: 12
            flat: true
            visible: root.text !== ""
            onClicked: { root.text = "" }
        }

        Button {
            text: qsTr("Today")
            font.pixelSize: 12
            flat: true
            onClicked: root.text = Qt.formatDate(new Date(), "MM/dd/yyyy")
        }
    }

    Rectangle {
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
        height: 1
        color: palette.placeholderText
        opacity: 0.3
    }

    DatePickerPopup {
        id: picker
        // parent: Overlay.overlay makes x/y relative to the full-screen overlay
        // rather than the 44px DateFieldRow, so bottom-of-screen positioning works.
        parent: Overlay.overlay
        onDateSelected: function(dateStr) { root.text = dateStr }
    }
}

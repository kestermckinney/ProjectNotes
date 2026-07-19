// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-Only

import QtQuick
import QtQuick.Controls

TextField {
    horizontalAlignment: TextInput.AlignLeft
    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
    leftPadding: 16
    rightPadding: 16
    background: Item {}

    Component.onCompleted: {
        cursorPosition = 0
    }
    onActiveFocusChanged: {
        if (!activeFocus) {
            // Small delay helps in some cases where text is being updated
            Qt.callLater(function() {
                cursorPosition = 0
            })
        }
    }
}
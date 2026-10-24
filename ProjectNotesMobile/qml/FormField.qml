// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-Only

import QtQuick
import QtQuick.Controls

TextField {
    id: control
    horizontalAlignment: TextInput.AlignLeft
    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 12; rightMargin: 12 }
    height: 36
    leftPadding: 12
    rightPadding: 12
    background: GlassField { focused: control.activeFocus }

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
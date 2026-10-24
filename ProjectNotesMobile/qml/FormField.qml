// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-Only

import QtQuick
import QtQuick.Controls

TextField {
    id: control
    horizontalAlignment: TextInput.AlignLeft
    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
    height: 50
    leftPadding: 18
    rightPadding: 18
    font.pixelSize: Theme.body; font.letterSpacing: Theme.tracking(Theme.body)
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
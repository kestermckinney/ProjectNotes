// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// Background shared by every text field, combo box and date field: a filled
// card in the grouped-settings color, as round as its height allows (a pill
// for one line, rounded corners for a text area), with a thin monochrome
// ring while focused.
//
//     TextField { id: field; background: GlassField { focused: field.activeFocus } }
Rectangle {
    id: root

    property bool focused: false
    property bool pressed: false

    implicitWidth: 120
    implicitHeight: 50
    radius: Math.min(height / 2, 22)
    color: pressed ? Qt.darker(Theme.cardBackground, Theme.isDarkMode ? 0.8 : 1.06)
                   : Theme.cardBackground
    border.width: focused ? 1.5 : 0
    border.color: Theme.focusRing

    Behavior on color { ColorAnimation { duration: 100 } }
}

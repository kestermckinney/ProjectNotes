// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls

// FormField for a SettingsItem row (fillControl: true): no box of its own,
// since the card is the box, and the text sits at the right edge of the row
// opposite the label, as in the iOS Settings app.
FormField {
    anchors.leftMargin: 0
    anchors.rightMargin: 0
    leftPadding: 0
    rightPadding: 0
    horizontalAlignment: TextInput.AlignRight
    font.pixelSize: Theme.body; font.letterSpacing: Theme.tracking(Theme.body)
    background: Item {}
}

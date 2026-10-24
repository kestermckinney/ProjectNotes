// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls

// FormCombo for a SettingsItem row (fillControl: true): no box of its own,
// the choice right-aligned against the style's arrow, opposite the label.
FormCombo {
    anchors.leftMargin: 0
    anchors.rightMargin: 0
    font.pixelSize: Theme.body; font.letterSpacing: Theme.tracking(Theme.body)
    background: Item {}

    // The iOS style's contentItem is a TextField with no alignment property
    // of its own on the combo, so set it once it exists.
    Component.onCompleted: contentItem.horizontalAlignment = TextInput.AlignRight
}

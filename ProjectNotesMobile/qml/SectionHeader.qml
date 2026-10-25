// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Title above a field on the detail pages: gray, semibold, body-sized, and
// lined up with the text inside the field card below it.
Label {
    Layout.fillWidth: true
    Layout.topMargin: 22
    leftPadding: 34          // 16 card margin + 18 text inset
    rightPadding: 16
    bottomPadding: 8
    font.pixelSize: Theme.headline; font.letterSpacing: Theme.tracking(Theme.headline)
    font.weight: Font.DemiBold
    color: Theme.secondaryText
    elide: Text.ElideRight
}

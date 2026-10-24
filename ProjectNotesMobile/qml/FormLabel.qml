// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-Only

import QtQuick
import QtQuick.Controls

Label {
    color: Theme.mutedText
    font.pixelSize: Theme.body; font.letterSpacing: Theme.tracking(Theme.body)
    leftPadding: 34
    rightPadding: 34
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight

    anchors {
        left: parent.left
        right: parent.right
        verticalCenter: parent.verticalCenter
    }
    height: parent.height
}

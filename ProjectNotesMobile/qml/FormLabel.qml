// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-Only

import QtQuick
import QtQuick.Controls

Label {
    color: Theme.mutedText
    font.pixelSize: 16
    leftPadding: 16
    rightPadding: 16
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight

    anchors {
        left: parent.left
        right: parent.right
        verticalCenter: parent.verticalCenter
    }
    height: parent.height
}

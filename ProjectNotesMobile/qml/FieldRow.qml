// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-Only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    default property alias content: innerItem.data

    Layout.fillWidth: true
    Layout.preferredHeight: 44
    color: palette.base

    Item {
        id: innerItem
        anchors.fill: parent
    }

    Rectangle {
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
        height: 1
        color: Theme.mutedText
        opacity: 0.3
    }
}

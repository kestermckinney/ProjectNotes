// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-Only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    default property alias content: innerItem.data

    Layout.fillWidth: true
    Layout.preferredHeight: 50
    color: "transparent"

    Item {
        id: innerItem
        anchors.fill: parent
    }

}

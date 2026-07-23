// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import ProjectNotesDesktop

// Placeholder for screens ported in later phases. Keeps the shell navigable.
Item {
    id: page
    property string pageTitle: "Coming soon"
    property string pageIcon: "hourglass_empty"

    Column {
        anchors.centerIn: parent
        spacing: 12
        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: page.pageIcon; size: 48; color: Theme.text3
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: page.pageTitle
            color: Theme.text
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "This screen arrives in a later phase of the UI overhaul."
            color: Theme.text2
            font.pixelSize: 13
        }
    }
}

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import ProjectNotesDesktop

// A three-dot ("kebab") button. Left-click surfaces a record's context/plugin
// menu — the same actions the row's right-click offers — for pointer users who
// don't think to right-click. Reports scene coordinates via clicked() so the
// caller can open a RecordContextMenu anchored just beneath the button.
Rectangle {
    id: btn

    // sceneX/sceneY: bottom-centre of the button in scene (overlay) coordinates,
    // ready to hand straight to RecordContextMenu.openAt().
    signal clicked(real sceneX, real sceneY)

    implicitWidth: 26
    implicitHeight: 26
    radius: Theme.radiusSm
    color: hover.hovered ? Theme.surface2 : "transparent"

    MaterialIcon {
        anchors.centerIn: parent
        name: "more_vert"
        size: 18
        color: hover.hovered ? Theme.text : Theme.text3
    }

    HoverHandler { id: hover }
    TapHandler {
        onTapped: {
            var p = btn.mapToItem(null, btn.width / 2, btn.height)
            btn.clicked(p.x, p.y)
        }
    }
}

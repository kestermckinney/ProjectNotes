// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls

// Round glass ≡ / back button that sits at the left of a page's top row,
// beside its search capsule or action pill. The four tab pages set
// `atRoot: true` and get the menu button; every other page gets the back
// chevron. Every page carries its own; the header shows only the title.
BarButton {
    property bool atRoot: false

    z: 2
    glass: true
    icon.width: 20; icon.height: 20
    icon.name: atRoot ? "line.3.horizontal" : "chevron.left"
    onClicked: {
        var win = ApplicationWindow.window
        if (!win) return
        if (atRoot)
            win.openMainMenu()
        else
            win.trySaveAndPop()
    }
}

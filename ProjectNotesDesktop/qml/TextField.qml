// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic as Controls

// Like Button.qml, this file lives in the app's QML module, so a plain
// `TextField { ... }` resolves to it instead of the Basic style's, in any
// file that imports ProjectNotesDesktop (or, in plugin QML,
// ProjectNotes.PluginUI) after QtQuick.Controls. The later import wins.
//
// TextInput scrolls to keep the cursor in view, and setting `text` puts the
// cursor at the end. A value loaded into a field too narrow for it, such as a
// path or a long name, therefore showed its end with the start cut off, and
// looked right-aligned. When the text changes while nobody is editing the
// field, the cursor goes back to the start so the beginning shows.
//
// Losing focus leaves the cursor alone: Insert Field, Paste Unformatted and
// the spell checker open a menu or dialog that takes focus, and still need
// the cursor and selection where the user left them.
Controls.TextField {
    function _showStart() {
        if (!activeFocus && cursorPosition !== 0)
            cursorPosition = 0
    }

    onTextChanged: _showStart()
    Component.onCompleted: _showStart()
}

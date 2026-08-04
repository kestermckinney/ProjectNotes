// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import "MaterialIconCodepoints.js" as Icons

// A single Material Symbols Rounded glyph. Pass the icon's name as `name`
// (e.g. "settings", "star", "folder"). Rendered by looking the name up in the
// font's private-use-area codepoint table and setting `text` to that
// character directly — NOT via OpenType ligature substitution (font.features:
// liga/rlig), which Windows' default DirectWrite text-shaping path doesn't
// apply the same way FreeType/HarfBuzz does on Linux, leaving icons blank or
// showing raw ligature names there. A direct codepoint is just a glyph
// lookup, so it renders identically on every platform/font backend.
Text {
    id: icon
    property string name: ""
    property int size: 20

    // Loaded once, shared across all instances.
    FontLoader {
        id: fontLoader
        source: "qrc:/qt/qml/ProjectNotesDesktop/resources/MaterialSymbolsRounded.ttf"
    }

    text: Icons.codepoints[name] !== undefined
          ? String.fromCodePoint(Icons.codepoints[name])
          : ""
    font.family: fontLoader.name
    font.pixelSize: size
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
    renderType: Text.QtRendering
}

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// A single Material Symbols Rounded glyph. Pass the icon's ligature name as
// `name` (e.g. "settings", "star", "folder"). The bundled variable font renders
// the name into its glyph via the OpenType `liga` feature.
Text {
    id: icon
    property string name: ""
    property int size: 20

    // Loaded once, shared across all instances.
    FontLoader {
        id: fontLoader
        source: "qrc:/qt/qml/ProjectNotesDesktop/resources/MaterialSymbolsRounded.woff2"
    }

    text: name
    font.family: fontLoader.name
    font.pixelSize: size
    // Material Symbols relies on standard ligatures to map names to glyphs.
    font.features: { "liga": 1 }
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
    renderType: Text.QtRendering
}

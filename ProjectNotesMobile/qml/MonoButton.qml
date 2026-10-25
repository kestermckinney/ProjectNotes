// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls

// Text button in the app's monochrome tint. The resting and disabled colors
// come from the window palette (see Main.qml); this replaces the pressed
// color, which the iOS style takes from palette.highlight (system blue).
// highlight can't be changed for the whole window because text selection and
// the sheets' checkmarks use it too.
Button {
    palette.highlight: Theme.iconColorPressed
}

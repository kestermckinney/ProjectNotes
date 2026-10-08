// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: control

    function navigatePage(direction) {
        if (!popup.visible) popup.open()
        var list = popup.contentItem
        var rowHeight = list && list.contentHeight > 0 && count > 0
                ? list.contentHeight / count : 30
        var rows = Math.max(1, Math.floor(list.height / rowHeight))
        for (var i = 0; i < rows; ++i) {
            if (direction > 0) incrementCurrentIndex()
            else decrementCurrentIndex()
        }
        if (list && highlightedIndex >= 0)
            list.positionViewAtIndex(highlightedIndex, ListView.Contain)
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_PageUp || event.key === Qt.Key_PageDown) {
            navigatePage(event.key === Qt.Key_PageDown ? 1 : -1)
            event.accepted = true
        }
    }
}

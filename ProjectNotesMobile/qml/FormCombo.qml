// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-Only

import QtQuick
import QtQuick.Controls

// ComboBox sized and anchored to sit in a FieldRow.
//
// Set `includeNone: true` on a field whose column permits an empty value — a
// "(none)" row is prepended so a value that has been set can be cleared again
// (matches the desktop ComboField's includeNone). Without it the combo offers
// the real choices only, and a value can never be taken back out by hand.
//
// `options` holds the real choices. Read the selection through `optionIndex` /
// `selection` and set it through selectOption() / selectText(), so callers
// never have to account for the extra row.
ComboBox {
    id: control

    property var    options:     []
    property bool   includeNone: false
    property string noneLabel:   qsTr("(none)")

    readonly property int _offset: includeNone ? 1 : 0

    // Position of the current selection within `options`; -1 on "(none)" or
    // no selection at all.
    readonly property int optionIndex: currentIndex >= _offset ? currentIndex - _offset : -1

    // Value of the current selection; "" on "(none)" or no selection.
    readonly property string selection:
        (optionIndex >= 0 && optionIndex < options.length) ? options[optionIndex] : ""

    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }

    model: includeNone ? [noneLabel].concat(options) : options

    // Select options[i]. i < 0 lands on the "(none)" row, or on no selection at
    // all for a combo that doesn't have one.
    function selectOption(i) {
        currentIndex = (i >= 0) ? i + _offset : (includeNone ? 0 : -1)
    }

    // Select by value. A value that isn't in `options` selects `fallbackIndex`,
    // or nothing (the "(none)" row, where there is one) when no fallback is
    // given — pass 0 for a required field that should land on its first option
    // rather than come up empty.
    function selectText(t, fallbackIndex) {
        var i = options.indexOf(t)
        selectOption(i >= 0 ? i : (fallbackIndex === undefined ? -1 : fallbackIndex))
    }
}

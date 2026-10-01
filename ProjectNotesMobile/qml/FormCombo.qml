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
    property bool   searchable:  false
    property string noneLabel:   qsTr("(none)")
    property string _filter: ""
    property bool _syncingSearch: false

    readonly property int _offset: includeNone ? 1 : 0

    // Position of the current selection within `options`; -1 on "(none)" or
    // no selection at all.
    readonly property int optionIndex: currentIndex >= _offset ? currentIndex - _offset : -1

    // Value of the current selection; "" on "(none)" or no selection.
    readonly property string selection:
        (optionIndex >= 0 && optionIndex < options.length) ? options[optionIndex] : ""

    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }

    model: includeNone ? [noneLabel].concat(options) : options
    editable: searchable

    // Keep the model stable while typing so optionIndex continues to refer to
    // the original list (and callers can still save the corresponding ID).
    onEditTextChanged: {
        if (searchable && activeFocus && !_syncingSearch) {
            _filter = editText.toLowerCase()
            if (!popup.visible)
                popup.open()
        }
    }
    delegate: ItemDelegate {
        required property int index
        required property var modelData
        readonly property bool matches: control._filter === ""
            || String(modelData).toLowerCase().indexOf(control._filter) >= 0
        width: control.width
        visible: matches
        height: matches ? implicitHeight : 0
        text: String(modelData)
    }

    function acceptSearch() {
        _syncingSearch = true
        var typed = editText.trim().toLowerCase()
        var match = -1
        if (typed === "" && includeNone)
            match = 0
        else {
            for (var i = _offset; i < model.length; ++i) {
                if (String(model[i]).toLowerCase() === typed) {
                    match = i
                    break
                }
            }
            if (match < 0) {
                for (var j = _offset; j < model.length; ++j) {
                    if (String(model[j]).toLowerCase().indexOf(typed) >= 0) {
                        match = j
                        break
                    }
                }
            }
        }
        _filter = ""
        popup.close()
        if (match >= 0) {
            currentIndex = match
            activated(match)
        } else {
            editText = currentIndex >= 0 ? textAt(currentIndex) : ""
        }
        _syncingSearch = false
    }
    onAccepted: if (searchable) acceptSearch()
    onActiveFocusChanged: {
        if (!activeFocus && searchable && _filter !== "")
            acceptSearch()
    }

    // Select options[i]. i < 0 lands on the "(none)" row, or on no selection at
    // all for a combo that doesn't have one.
    function selectOption(i) {
        _syncingSearch = true
        currentIndex = (i >= 0) ? i + _offset : (includeNone ? 0 : -1)
        _syncingSearch = false
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

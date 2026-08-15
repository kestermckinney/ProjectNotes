// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Labeled combo box. `options` is a string list; two-way value via currentText.
//
// Set `includeNone: true` to prepend a "none" entry (e.g. for optional person
// selectors). Choosing it emits activated("") and an empty `value` selects it.
//
// Set `searchable: true` to make the box editable with type-to-search: the user
// can clear it and start typing, and the drop-down hides everything that doesn't
// match. Used for combos backed by database lookups (people, clients) where the
// list is long. Free-typed text that matches no entry is reverted on focus loss.
ColumnLayout {
    id: root
    property string label: ""
    property var options: []
    property string value: ""
    property bool includeNone: false
    property bool searchable: false
    property string noneLabel: qsTr("(none)")
    signal activated(string value)

    // The model shown — options, optionally with the none entry first. This stays
    // stable while the user types; searching only hides rows, never swaps models.
    readonly property var _model: includeNone ? [noneLabel].concat(options) : options

    spacing: 3
    Layout.fillWidth: true

    Text {
        text: root.label
        visible: root.label !== ""
        color: Theme.text3
        font.pixelSize: 10
        font.weight: Font.DemiBold
    }

    ComboBox {
        id: combo
        Layout.fillWidth: true
        implicitHeight: 30
        editable: root.searchable
        model: root._model
        font.pixelSize: 12

        // Current type-to-search text (lower-cased match target). Empty = show all.
        property string _filter: ""

        // Sync external value → selection without clobbering user edits.
        property bool _syncing: false
        function _syncFromValue() {
            _syncing = true
            _filter = ""
            var idx
            // Empty value maps to the none entry when present.
            if (root.includeNone && root.value === "")
                idx = 0
            else
                idx = root._model.indexOf(root.value)
            currentIndex = idx
            if (editable)
                editText = idx >= 0 ? root._model[idx] : ""
            _syncing = false
        }

        // Commit a chosen/typed value. Blank (or the none entry) emits "";
        // anything that isn't a known option is reverted to the current value.
        function _commit(text) {
            if (_syncing) return
            _filter = ""
            if (root.includeNone && (text === "" || text === root.noneLabel)) {
                root.value = ""
                root.activated("")
            } else if (root._model.indexOf(text) >= 0) {
                root.value = text
                root.activated(text)
            } else {
                _syncFromValue() // ignore free text that matches no entry
            }
        }

        // Only fires when the option list itself changes (e.g. DB names load),
        // not while typing — filtering no longer touches the model.
        onModelChanged: if (!_syncing) _syncFromValue()
        Component.onCompleted: _syncFromValue()
        Connections {
            target: root
            function onValueChanged() { combo._syncFromValue() }
        }
        onActivated: (i) => _commit(textAt(i)) // popup pick / keyboard select
        onAccepted: _commit(editText)          // Enter typed in the editor
        // When focus leaves: honor a cleared box (for a none-able combo that means
        // committing "(none)" so the clear actually saves; otherwise revert), and
        // discard any uncommitted partial search text so the box shows a real value.
        onActiveFocusChanged: {
            if (activeFocus)
                return
            if (editable && editText === "")
                root.includeNone ? _commit("") : _syncFromValue()
            else if (_filter !== "")
                _syncFromValue()
        }

        contentItem: TextField {
            leftPadding: 9
            rightPadding: 26
            text: combo.editable ? combo.editText : combo.displayText
            enabled: combo.editable          // disabled → clicks open the popup
            // Stay editable even while the search popup is open. (Binding to
            // combo.down would go read-only the moment the popup shows, blocking
            // every keystroke after the first.)
            readOnly: !combo.editable
            color: Theme.text
            font: combo.font
            verticalAlignment: Text.AlignVCenter
            selectByMouse: true
            background: null
            // Select all on focus so typing immediately replaces the current
            // value — the user can search without hand-clearing the box first.
            onActiveFocusChanged: if (activeFocus) selectAll()
            onTextEdited: {
                combo._filter = text
                if (!combo.popup.visible)
                    combo.popup.open()
            }
        }
        background: Rectangle {
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: combo.activeFocus ? Theme.accent : Theme.border
        }
        indicator: MaterialIcon {
            name: "expand_more"
            size: 16
            color: Theme.text3
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 7
        }
        popup: Popup {
            y: combo.height + 2
            width: combo.width
            implicitHeight: Math.min(contentItem.implicitHeight + 2, 280)
            padding: 1
            background: Rectangle {
                radius: Theme.radiusSm
                color: Theme.raise
                border.color: Theme.border
            }
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                ScrollIndicator.vertical: ScrollIndicator {}
            }
        }
        delegate: ItemDelegate {
            id: itemDelegate
            width: combo.width
            required property int index
            required property var modelData
            // Type-to-search: collapse rows that don't contain the filter text.
            readonly property bool _match: combo._filter === ""
                || String(modelData).toLowerCase().indexOf(combo._filter.toLowerCase()) >= 0
            visible: _match
            height: _match ? implicitHeight : 0
            contentItem: Text {
                text: itemDelegate.modelData
                color: Theme.text
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: itemDelegate.highlighted ? Theme.accentSoft : "transparent"
            }
            highlighted: combo.highlightedIndex === itemDelegate.index
        }
    }
}

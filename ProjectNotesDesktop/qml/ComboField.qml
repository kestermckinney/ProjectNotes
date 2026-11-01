// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
// After QtQuick.Controls.Basic, so TextField is the app's (TextField.qml).
import ProjectNotesDesktop

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
    readonly property Item inputItem: root.searchable ? comboEditor : combo
    property Item tabNext: null
    property Item tabPrevious: null
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
        font.pixelSize: Theme.fontXs
        font.weight: Font.DemiBold
    }

    KeyboardComboBox {
        id: combo
        objectName: root.label + "Input"
        KeyNavigation.tab: root.tabNext
        KeyNavigation.backtab: root.tabPrevious
        Layout.fillWidth: true
        implicitHeight: 30
        editable: root.searchable
        model: root._model
        font.pixelSize: Theme.fontBody
        displayText: root.includeNone && currentIndex === 0 ? "" : currentText

        // Current type-to-search text. Empty = show all.
        property string _filter: ""
        // Keep keyboard navigation separate from currentIndex: moving through
        // suggestions must not save a value until the user accepts one.
        property int _keyboardIndex: -1

        function _matches(i) {
            return String(root._model[i]).toLowerCase().indexOf(_filter.toLowerCase()) >= 0
        }

        function _moveHighlight(step) {
            if (!popup.visible)
                popup.open()
            var start = _keyboardIndex < 0
                ? (step > 0 ? -1 : root._model.length) : _keyboardIndex
            for (var i = start + step; i >= 0 && i < root._model.length; i += step) {
                if (_matches(i)) {
                    _keyboardIndex = i
                    suggestionList.positionViewAtIndex(i, ListView.Contain)
                    return
                }
            }
        }

        function _movePage(direction) {
            if (!popup.visible) popup.open()
            var rowHeight = Math.max(comboEditor.font.pixelSize + 16, 30)
            var rows = Math.max(1, Math.floor(suggestionList.height / rowHeight))
            for (var i = 0; i < rows; ++i) _moveHighlight(direction)
        }

        // Sync external value → selection without clobbering user edits.
        property bool _syncing: false
        function _syncFromValue() {
            _syncing = true
            _filter = ""
            _keyboardIndex = -1
            var idx
            // Empty value maps to the none entry when present.
            if (root.includeNone && root.value === "")
                idx = 0
            else
                idx = root._model.indexOf(root.value)
            currentIndex = idx
            if (editable)
                editText = idx >= 0 && !(root.includeNone && idx === 0)
                    ? root._model[idx] : ""
            _syncing = false
        }

        // Resolve typed text to an option. An explicitly highlighted row wins;
        // otherwise prefer an exact match, Qt's highlighted row, then the first
        // visible result.
        function _completionFor(text) {
            var needle = String(text).toLowerCase()
            if (needle === "")
                return ""

            if (_keyboardIndex >= 0 && _matches(_keyboardIndex))
                return root._model[_keyboardIndex]

            var i
            for (i = root.includeNone ? 1 : 0; i < root._model.length; ++i) {
                if (String(root._model[i]).toLowerCase() === needle)
                    return root._model[i]
            }

            if (highlightedIndex >= (root.includeNone ? 1 : 0)
                    && _matches(highlightedIndex))
                return root._model[highlightedIndex]

            for (i = root.includeNone ? 1 : 0; i < root._model.length; ++i) {
                if (String(root._model[i]).toLowerCase().indexOf(needle) >= 0)
                    return root._model[i]
            }
            return ""
        }

        function _acceptTypedText() {
            var typed = editText
            var completion = _completionFor(typed)
            popup.close()
            if (typed === "" && root.includeNone)
                _commit("")
            else if (completion !== "")
                _commit(completion)
            else
                _syncFromValue()
        }

        // Commit a chosen/typed value. Blank (or the none entry) emits "";
        // anything that isn't a known option is reverted to the current value.
        function _commit(text) {
            if (_syncing) return
            _filter = ""
            if (root.includeNone && (text === "" || text === root.noneLabel)) {
                root.value = ""
                root.activated("")
                _syncFromValue()
            } else if (root._model.indexOf(text) >= 0) {
                root.value = text
                root.activated(text)
                _syncFromValue()
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
        // When focus leaves: honor a cleared box (for a none-able combo that means
        // committing a blank value so the clear actually saves; otherwise revert),
        // and accept the same autocomplete result as Return.
        onActiveFocusChanged: {
            if (activeFocus)
                return
            if (editable && editText === "")
                root.includeNone ? _commit("") : _syncFromValue()
            else if (_filter !== "")
                _acceptTypedText()
        }

        contentItem: TextField {
            id: comboEditor
            KeyNavigation.tab: root.tabNext
            KeyNavigation.backtab: root.tabPrevious
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
                combo._keyboardIndex = -1
                combo._filter = text
                if (!combo.popup.visible)
                    combo.popup.open()
            }
            Keys.onDownPressed: (event) => {
                if (!combo.editable) return
                combo._moveHighlight(1)
                event.accepted = true
            }
            Keys.onUpPressed: (event) => {
                if (!combo.editable) return
                combo._moveHighlight(-1)
                event.accepted = true
            }
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_PageUp || event.key === Qt.Key_PageDown) {
                    var direction = event.key === Qt.Key_PageDown ? 1 : -1
                    if (combo.editable) combo._movePage(direction)
                    else combo.navigatePage(direction)
                    event.accepted = true
                }
            }
            Keys.onReturnPressed: (event) => {
                combo._acceptTypedText()
                event.accepted = true
            }
            Keys.onEnterPressed: (event) => {
                combo._acceptTypedText()
                event.accepted = true
            }
            // Only an editable combo has text of its own to cut and paste; on a
            // fixed list the box is a label, so it keeps the list's own clicks.
            Loader {
                active: combo.editable
                anchors.fill: parent
                // The Loader, not the field, is this item's parent, so the
                // editor is named rather than defaulted.
                sourceComponent: SpellCheckField { target: comboEditor; spellCheckEnabled: false }
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
        // A pick (or a click away) must not also reach what's under the list —
        // see ClickShield.qml. z: 1 keeps the list, and its shield at 0.5, above
        // a dialog the combo sits in.
        popup: Popup {
            id: listPopup
            z: 1
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
                id: suggestionList
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                ScrollIndicator.vertical: ScrollIndicator {}
            }
            ClickShield { host: listPopup }
        }

        // Mouse pick from the list. Deferred to the next tick: closing the popup
        // nulls the list model above, destroying the clicked row mid-click, and
        // the orphaned release then falls through to whatever sat under the list.
        function _pick(i) {
            var text = textAt(i)
            Qt.callLater(function() {
                combo.currentIndex = i
                combo.popup.close()
                combo._commit(text)
                combo._syncFromValue()
            })
        }

        // A plain Item, not an ItemDelegate: ComboBox closes its popup
        // synchronously when a button delegate is clicked, which is the leak
        // _pick() avoids. The editor handles keyboard selection separately.
        delegate: Rectangle {
            id: itemDelegate
            width: combo.width
            required property int index
            required property var modelData
            // Type-to-search: collapse rows that don't contain the filter text.
            readonly property bool _match: combo._filter === ""
                || String(modelData).toLowerCase().indexOf(combo._filter.toLowerCase()) >= 0
            visible: _match
            // Text fills this rectangle, so its implicitHeight would depend on
            // this height and create a binding loop while rows collapse.
            height: _match ? Math.max(rowText.font.pixelSize + 16, 30) : 0
            color: itemHover.hovered || (combo._keyboardIndex >= 0
                ? combo._keyboardIndex === index : combo.highlightedIndex === index)
                ? Theme.accentSoft : "transparent"
            Text {
                id: rowText
                anchors.fill: parent
                leftPadding: 12; rightPadding: 12
                text: itemDelegate.modelData
                color: Theme.text
                font.pixelSize: Theme.fontBody
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            HoverHandler { id: itemHover }
            TapHandler {
                gesturePolicy: TapHandler.ReleaseWithinBounds
                onTapped: combo._pick(itemDelegate.index)
            }
        }
    }
}

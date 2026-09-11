// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Modal dialog for picking one person from a list, with type-to-search and
// keyboard navigation. Type to filter; Down/Up move the highlight through the
// matching names; Return picks the highlighted row; Escape (or a click away)
// dismisses. Used everywhere a person is chosen from a list — the project Team
// roster and a meeting note's Attendees.
//
//   PeoplePickerDialog {
//       id: teamPicker
//       headingText: qsTr("Add Team Member")
//       model: DesktopAppController.peopleList()
//       onPicked: (person) => { ... person.id ... person.name ... }
//   }
Dialog {
    id: root

    // Bold title shown at the top of the dialog.
    property string headingText: qsTr("Add Person")

    // A list of objects, each exposing `name` and `id` (e.g. the QVariantList
    // returned by DesktopAppController.peopleList() / teamMemberList()).
    property alias model: peopleList.model

    // Optional function called each time the dialog opens; its return value
    // replaces `model`. Use it when the underlying list can change while the
    // page that owns this picker stays alive (e.g. a project's team roster).
    property var reload: null

    // Emitted with the chosen row's object when the user picks someone. The
    // dialog closes itself immediately afterward.
    signal picked(var person)

    anchors.centerIn: parent
    width: 320
    height: 380
    modal: true
    padding: 0
    scale: Theme.uiScale   // match the zoomed workspace (centered origin)
    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

    // Clicking away dismisses the picker and nothing else — see ClickShield.qml.
    ClickShield { host: root }

    // Type-to-search text (lower-cased match target). Empty = show everyone.
    property string _filter: ""

    // Model indices that currently match _filter, in list order. Recomputed
    // whenever the filter or the model changes.
    property var _visible: []

    // Position within _visible that the keyboard highlight sits on.
    property int _hi: 0

    onModelChanged: _recompute()
    onOpened: {
        if (reload)
            peopleList.model = reload()
        _filter = ""
        searchField.text = ""
        _hi = 0
        _recompute()
        searchField.forceActiveFocus()
    }

    function _recompute() {
        var rows = []
        var m = peopleList.model
        var n = m ? m.length : 0
        var needle = _filter.toLowerCase()
        for (var i = 0; i < n; i++) {
            if (needle === ""
                || String(m[i].name).toLowerCase().indexOf(needle) >= 0)
                rows.push(i)
        }
        _visible = rows
        _hi = Math.max(0, Math.min(_hi, rows.length - 1))
    }

    // Move the highlight by delta rows through the visible matches.
    function _move(delta) {
        if (_visible.length === 0)
            return
        _hi = Math.max(0, Math.min(_visible.length - 1, _hi + delta))
        peopleList.positionViewAtIndex(_visible[_hi], ListView.Contain)
    }

    function _pick(modelIndex) {
        if (modelIndex < 0 || !peopleList.model || modelIndex >= peopleList.model.length)
            return
        root.picked(peopleList.model[modelIndex])
        root.close()
    }

    function _pickHighlighted() {
        if (_visible.length > 0)
            _pick(_visible[_hi])
    }

    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            Text {
                text: root.headingText
                color: Theme.text
                font.pixelSize: Theme.fontXl
                font.weight: Font.Bold
                Layout.fillWidth: true
            }
            MaterialIcon {
                name: "close"; size: 18; color: Theme.text3
                TapHandler { onTapped: root.close() }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        // Search field — filters the list below as you type, and drives the
        // Up/Down/Return keyboard selection.
        Rectangle {
            Layout.fillWidth: true
            Layout.margins: 10
            implicitHeight: 30
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: searchField.activeFocus ? Theme.accent : Theme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 9; anchors.rightMargin: 9
                spacing: 5
                MaterialIcon { name: "search"; size: 14; color: Theme.text3; Layout.alignment: Qt.AlignVCenter }
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search people…")
                    placeholderTextColor: Theme.text3
                    color: Theme.text
                    font.pixelSize: Theme.fontBody
                    background: null
                    verticalAlignment: Text.AlignVCenter
                    selectByMouse: true
                    onTextChanged: {
                        root._filter = text
                        root._hi = 0
                        root._recompute()
                    }
                    Keys.onDownPressed: root._move(1)
                    Keys.onUpPressed: root._move(-1)
                    Keys.onReturnPressed: root._pickHighlighted()
                    Keys.onEnterPressed: root._pickHighlighted()
                }
            }
        }

        ListView {
            id: peopleList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            delegate: ItemDelegate {
                id: personDelegate
                required property int index
                required property var modelData
                // Collapse rows that don't contain the search text.
                readonly property bool _match: root._visible.indexOf(index) >= 0
                readonly property bool _isHi: root._visible.length > 0
                    && root._visible[root._hi] === index
                visible: _match
                width: peopleList.width
                height: _match ? 34 : 0
                contentItem: Text {
                    text: personDelegate.modelData.name
                    color: Theme.text
                    font.pixelSize: Theme.fontBody
                    leftPadding: 12
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: personDelegate._isHi ? Theme.accentSoft
                         : personDelegate.hovered ? Theme.surface2
                         : "transparent"
                }
                // Keep the keyboard highlight in step with the mouse.
                onHoveredChanged: {
                    if (!hovered)
                        return
                    var p = root._visible.indexOf(index)
                    if (p >= 0)
                        root._hi = p
                }
                onClicked: root._pick(index)
            }
        }
    }
}

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Labeled combo box. `options` is a string list; two-way value via currentText.
//
// Set `includeNone: true` to prepend a "none" entry (e.g. for optional person
// selectors). Choosing it emits activated("") and an empty `value` selects it.
ColumnLayout {
    id: root
    property string label: ""
    property var options: []
    property string value: ""
    property bool includeNone: false
    property string noneLabel: qsTr("(none)")
    signal activated(string value)

    // The model actually shown — options, optionally with the none entry first.
    readonly property var _model: includeNone ? [noneLabel].concat(options) : options

    spacing: 4
    Layout.fillWidth: true

    Text {
        text: root.label
        visible: root.label !== ""
        color: Theme.text3
        font.pixelSize: 11
        font.weight: Font.DemiBold
    }

    ComboBox {
        id: combo
        Layout.fillWidth: true
        implicitHeight: 34
        model: root._model
        font.pixelSize: 13

        // Sync external value → selection without clobbering user edits.
        property bool _syncing: false
        function _syncFromValue() {
            _syncing = true
            // Empty value maps to the none entry when present.
            if (root.includeNone && root.value === "")
                currentIndex = 0
            else
                currentIndex = root._model.indexOf(root.value)
            _syncing = false
        }
        onModelChanged: _syncFromValue()
        Component.onCompleted: _syncFromValue()
        Connections {
            target: root
            function onValueChanged() { combo._syncFromValue() }
        }
        onActivated: (i) => {
            if (_syncing) return
            if (root.includeNone && i === 0) root.activated("")
            else root.activated(root._model[i])
        }

        contentItem: Text {
            leftPadding: 10
            text: combo.displayText
            color: Theme.text
            font: combo.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: combo.activeFocus ? Theme.accent : Theme.border
        }
        indicator: MaterialIcon {
            name: "expand_more"
            size: 18
            color: Theme.text3
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 8
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
            width: combo.width
            required property int index
            required property var modelData
            contentItem: Text {
                text: modelData
                color: Theme.text
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: highlighted ? Theme.accentSoft : "transparent"
            }
            highlighted: combo.highlightedIndex === index
        }
    }
}

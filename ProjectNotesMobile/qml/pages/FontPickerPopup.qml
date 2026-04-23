// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// FontPickerPopup — bottom-sheet popup for choosing font family, size, and color.
// Usage:
//   FontPickerPopup {
//       id: fontPicker
//       parent: Overlay.overlay
//       onFontApplied: function(family, pointSize, textColor) { ... }
//   }
//   fontPicker.openWithFormat(family, size, color)

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    // Emitted when the user taps Done.
    signal fontApplied(string family, int pointSize, color textColor)

    // ── Public API ────────────────────────────────────────────────────────────
    // Deferred so indices are applied after the popup is fully open.
    property var _pending: null

    function openWithFormat(family, pointSize, textColor) {
        _pending = { family: family, size: pointSize, colorStr: textColor.toString() }
        open()
    }

    // Load font families once when the popup component is created — before any
    // user interaction.  Doing it here (instead of lazily inside onOpened) keeps
    // the model stable during the open animation so the ComboBox never reacts to
    // a model change while Qt's popup machinery is running, which triggered:
    //   ASSERT: "!isEmpty()" in qlist.h  (QList::first on an empty list)
    Component.onCompleted: {
        Qt.callLater(function() {
            _familyModel = TextFormatter.availableFontFamilies()
        })
    }

    onOpened: {
        if (_pending) {
            // Capture and clear _pending before the deferred callback fires.
            var p = _pending
            _pending = null

            // Defer index/size/color assignment one tick so the ComboBox has
            // finished its own open-animation before we mutate its currentIndex.
            Qt.callLater(function() {
                if (root._familyModel.length > 0) {
                    var fidx = root._familyModel.indexOf(p.family)
                    familyCombo.currentIndex = (fidx >= 0) ? fidx : 0
                }
                root._selectedSize = p.size > 0 ? p.size : 12

                // Normalize to #rrggbb — QColor.toString() may return #aarrggbb;
                // the preset swatches are stored without an alpha component.
                var c = p.colorStr
                root._selectedColorStr = (c.length === 9 && c[0] === "#") ? "#" + c.slice(3) : c
            })
        }
    }

    // ── Internal state ────────────────────────────────────────────────────────
    property int    _selectedSize:     12
    property string _selectedColorStr: "#000000"

    // Pre-populated in Component.onCompleted — stable for the lifetime of the popup.
    property var _familyModel: []

    property var _presetSizes: [8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 32, 36, 48, 72]

    // 20 common colors in 4 rows of 5
    property var _presetColors: [
        "#000000", "#404040", "#808080", "#c0c0c0", "#ffffff",
        "#800000", "#ff0000", "#ff6600", "#ff9900", "#ffff00",
        "#006600", "#00aa00", "#006666", "#00aaaa", "#00aaff",
        "#003399", "#0055ff", "#6600cc", "#cc00cc", "#ff66cc"
    ]

    // ── Popup geometry — slides up from screen bottom ─────────────────────────
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay

    x: 0
    property real slideY: 0
    y: (parent ? parent.height - height : 0) + slideY
    width:  parent ? parent.width : 390
    height: 480
    padding: 0

    enter: Transition {
        NumberAnimation { target: root; property: "slideY"; from: root.height; to: 0; duration: 280; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { target: root; property: "slideY"; from: 0; to: root.height; duration: 220; easing.type: Easing.InCubic }
    }

    background: Rectangle {
        color: palette.base
        radius: 16
        layer.enabled: true
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ── Header bar ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "transparent"

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                Button { text: qsTr("Cancel"); flat: true; onClicked: root.close() }
                Label {
                    text: qsTr("Font")
                    font.pixelSize: 16; font.bold: true
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }
                Button {
                    text: qsTr("Done"); flat: true; font.bold: true
                    onClicked: {
                        root.fontApplied(familyCombo.currentText,
                                         root._selectedSize,
                                         root._selectedColorStr)
                        root.close()
                    }
                }
            }
            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: palette.placeholderText; opacity: 0.3
            }
        }

        // ── Scrollable body ───────────────────────────────────────────────────
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 0

                // ── Font family ───────────────────────────────────────────────
                Label {
                    Layout.fillWidth: true
                    leftPadding: 16; topPadding: 14; bottomPadding: 4
                    text: qsTr("FONT FAMILY")
                    font.pixelSize: 12; font.weight: 500
                    color: palette.placeholderText
                }
                ComboBox {
                    id: familyCombo
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    model: root._familyModel
                }

                // ── Font size ─────────────────────────────────────────────────
                Label {
                    Layout.fillWidth: true
                    leftPadding: 16; topPadding: 16; bottomPadding: 6
                    text: qsTr("SIZE")
                    font.pixelSize: 12; font.weight: 500
                    color: palette.placeholderText
                }
                Flow {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    spacing: 6

                    Repeater {
                        model: root._presetSizes
                        delegate: Rectangle {
                            width: 44; height: 34
                            radius: 6
                            color: (modelData === root._selectedSize) ? palette.highlight : palette.button

                            Label {
                                anchors.centerIn: parent
                                text: modelData
                                font.pixelSize: 14
                                color: (modelData === root._selectedSize)
                                       ? palette.highlightedText : palette.buttonText
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root._selectedSize = modelData
                            }
                        }
                    }
                }

                // ── Font color ────────────────────────────────────────────────
                Label {
                    Layout.fillWidth: true
                    leftPadding: 16; topPadding: 16; bottomPadding: 8
                    text: qsTr("COLOR")
                    font.pixelSize: 12; font.weight: 500
                    color: palette.placeholderText
                }
                Flow {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    spacing: 8

                    Repeater {
                        model: root._presetColors
                        delegate: Rectangle {
                            readonly property bool isSelected: root._selectedColorStr === modelData
                            width: 36; height: 36; radius: 18
                            color: modelData
                            border.color: isSelected ? palette.highlight
                                                     : (modelData === "#ffffff" ? palette.mid : "transparent")
                            border.width: isSelected ? 3 : 1

                            MouseArea {
                                anchors.fill: parent
                                onClicked: root._selectedColorStr = modelData
                            }
                        }
                    }
                }

                // ── Live preview ──────────────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    Layout.topMargin: 16; Layout.bottomMargin: 8
                    height: 52
                    color: palette.window
                    radius: 8

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("Sample Text")
                        font.family:    familyCombo.currentText
                        font.pointSize: root._selectedSize
                        color:          root._selectedColorStr
                    }
                }

                Item { Layout.preferredHeight: 16 }
            }
        }
    }
}

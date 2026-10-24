// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Effects

// Floating glass capsule shared by the tab bar, the list search bars and the
// note editor's format bar: a soft shadow, a translucent tint, a hairline
// edge and a faint top sheen. Children go inside, inset by `contentPadding`.
//
// Set `blurSource` to the item drawn behind the bar, and `blurRect` to the
// bar's area in that item's coordinates, to frost whatever scrolls under it.
// Without a blur source the bar is a plain tinted capsule, which is right
// where nothing passes behind it.
Item {
    id: root

    default property alias content: contentArea.data
    property real radius: height / 2
    property real contentPadding: 5
    property Item blurSource: null
    property rect blurRect: Qt.rect(0, 0, width, height)

    readonly property bool _dark: Theme.isDarkMode

    // Soft drop shadow so the capsule reads as floating.
    RectangularShadow {
        anchors.fill: parent
        radius: root.radius
        offset.y: 6
        blur: 24
        spread: -2
        color: root._dark ? Qt.rgba(0, 0, 0, 0.45) : Qt.rgba(0, 0, 0, 0.14)
    }

    // Frosted backdrop: a live copy of what's behind the capsule, blurred and
    // clipped to its rounded shape.
    Loader {
        anchors.fill: parent
        active: root.blurSource !== null
        sourceComponent: Item {
            ShaderEffectSource {
                id: backdrop
                anchors.fill: parent
                sourceItem: root.blurSource
                sourceRect: root.blurRect
                live: true
                visible: false
            }
            Rectangle {
                id: capsuleMask
                anchors.fill: parent
                radius: root.radius
                visible: false
                layer.enabled: true
            }
            Item {
                anchors.fill: parent
                // Mask the blurred layer as a whole; masking inside the blur
                // effect itself leaves its padded edges unclipped.
                layer.enabled: true
                layer.effect: MultiEffect {
                    maskEnabled: true
                    maskSource: capsuleMask
                    maskThresholdMin: 0.5
                    maskSpreadAtMin: 1.0
                }
                // Solid base under the blur. Content with no background of
                // its own (form fields on the page) gives a see-through
                // blurred copy, and the sharp original would show through.
                Rectangle {
                    anchors.fill: parent
                    color: Theme.pageBackground
                }
                MultiEffect {
                    anchors.fill: parent
                    source: backdrop
                    // Keep the output inside the capsule; auto padding spreads
                    // the blur past its bounds and thins it out.
                    autoPaddingEnabled: false
                    blurEnabled: true
                    blur: 1.0
                    blurMax: 40
                    saturation: 0.35
                }
            }
        }
    }

    // Translucent tint and hairline edge, the same on every bar so they all
    // match the tab bar.
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root._dark ? Qt.rgba(0.11, 0.11, 0.12, 0.55) : Qt.rgba(1, 1, 1, 0.55)
        border.width: 1
        border.color: root._dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.7)
    }
    // Sheen: light catching the top edge of the glass.
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: root.radius - 1
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, root._dark ? 0.10 : 0.35) }
            GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.0) }
        }
    }

    Item {
        id: contentArea
        anchors.fill: parent
        anchors.margins: root.contentPadding
    }
}

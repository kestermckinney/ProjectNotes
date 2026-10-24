// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Appearance: follow the system's light/dark setting, or force Day or Night.
// The choice is saved by AppController and applied to the whole app at once.

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Effects
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Appearance")

    // Same spot as on the list pages; see MainMenuPage.qml.
    NavButton {
        id: navButton
        anchors { top: parent.top; left: parent.left; topMargin: Theme.topInset + 8; leftMargin: 12 }
    }

    ScrollView {
        anchors.fill: parent    // content scrolls up under the title bar
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            Item { Layout.preferredHeight: navButton.y + navButton.height + 20 }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 10

                ModeTile { mode: 0; label: qsTr("System"); iconName: "circle.lefthalf.filled" }
                ModeTile { mode: 1; label: qsTr("Day");    iconName: "sun.max.fill" }
                ModeTile { mode: 2; label: qsTr("Night");  iconName: "moon.fill" }
            }

            // ── Text size ─────────────────────────────────────────────────────
            // Seven steps, as in iOS Dynamic Type. The whole app rescales as
            // the slider moves, this page and its preview included.
            SettingsSection {
                title: qsTr("Text Size")
                Layout.topMargin: 32

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 18
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        spacing: 14

                        // Fixed sizes: the two ends of the scale, not scaled text.
                        Label { text: "A"; font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.secondaryText }
                        SizeSlider { Layout.fillWidth: true }
                        Label { text: "A"; font.pixelSize: 24; font.weight: Font.DemiBold; color: Theme.secondaryText }
                    }

                    // Preview: a list row and a field, drawn with the same sizes
                    // the lists and detail pages use.
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.margins: 20
                        Layout.topMargin: 22
                        spacing: 4

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("00042  Website Redesign")
                            font.pixelSize: Theme.headline; font.letterSpacing: Theme.tracking(Theme.headline)
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Acme Corporation")
                                font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
                                color: Theme.mutedText
                                elide: Text.ElideRight
                            }
                            Label {
                                text: "● " + qsTr("Active")
                                font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
                                color: Theme.accentGreenDark
                            }
                        }

                        Label {
                            Layout.topMargin: 16
                            leftPadding: 18
                            text: qsTr("Project Name")
                            font.pixelSize: Theme.headline; font.letterSpacing: Theme.tracking(Theme.headline)
                            font.weight: Font.DemiBold
                            color: Theme.secondaryText
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            Layout.preferredHeight: Math.max(50, previewField.implicitHeight + 24)
                            radius: Math.min(height / 2, 22)
                            color: Theme.pageBackground
                            Label {
                                id: previewField
                                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
                                          leftMargin: 18; rightMargin: 18 }
                                text: qsTr("Website Redesign")
                                font.pixelSize: Theme.body; font.letterSpacing: Theme.tracking(Theme.body)
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    // Seven-step slider in the style of the reference: a thin track filled
    // with the monochrome tint up to a wide white knob, with a dot under each
    // step.
    component SizeSlider: Slider {
        id: slider
        from: 0
        to: 6
        stepSize: 1
        snapMode: Slider.SnapAlways
        value: AppController.textSize
        onMoved: AppController.textSize = Math.round(value)

        background: Item {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 200
            implicitHeight: 4
            width: slider.availableWidth
            height: 4

            Rectangle {
                anchors.fill: parent
                radius: 2
                color: Theme.switchOffTrack
            }
            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: 2
                color: Theme.iconColor
            }
            Repeater {
                model: 7
                Rectangle {
                    // Under the track, one per step; the knob's travel is the
                    // track width less the knob, so the dots follow that span.
                    readonly property real span: parent.width - slider.handle.width
                    x: slider.handle.width / 2 + index * span / 6 - width / 2
                    y: 12
                    width: 4; height: 4; radius: 2
                    color: Theme.switchOffTrack
                }
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 44
            implicitHeight: 28
            radius: height / 2
            color: "#ffffff"
            scale: slider.pressed ? 1.08 : 1
            Behavior on scale { NumberAnimation { duration: 120 } }
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Qt.rgba(0, 0, 0, 0.25)
                shadowBlur: 0.4
                shadowVerticalOffset: 1
            }
        }
    }

    // A tile with the mode's symbol and its name underneath. The chosen tile
    // is filled; the others are outlined and dimmed.
    component ModeTile: ColumnLayout {
        id: tile

        property int mode: 0
        property string label: ""
        property string iconName: ""
        readonly property bool selected: AppController.appearance === mode

        Layout.fillWidth: true
        Layout.preferredWidth: 1      // three equal columns
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            radius: 20
            color: tile.selected ? Theme.cardBackground : "transparent"
            border.width: tile.selected ? 0 : 1
            border.color: Theme.separator
            scale: tap.pressed ? 0.96 : 1
            Behavior on scale { NumberAnimation { duration: 120 } }
            Behavior on color { ColorAnimation { duration: 150 } }

            IconImage {
                anchors.centerIn: parent
                width: 30; height: 30
                name: tile.iconName
                color: tile.selected ? Theme.iconColor : Theme.secondaryText
                sourceSize: Qt.size(30, 30)
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: tap
                anchors.fill: parent
                onClicked: AppController.appearance = tile.mode
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: tile.label
            font.pixelSize: Theme.subheadline; font.letterSpacing: Theme.tracking(Theme.subheadline)
            font.weight: tile.selected ? Font.DemiBold : Font.Normal
            color: tile.selected ? palette.text : Theme.secondaryText
        }
    }
}

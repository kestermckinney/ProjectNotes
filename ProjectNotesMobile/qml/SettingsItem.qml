// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

// One row of a SettingsSection card: an optional SF Symbol icon, the label,
// an optional gray value, and either a › chevron (rows that open a page) or
// a control such as a MonoSwitch:
//
//     SettingsItem { text: qsTr("View"); iconName: "eye"; onClicked: ... }
//     SettingsItem { text: qsTr("Enable Sync"); showChevron: false
//                    control: MonoSwitch { ... } }
//
// With `fillControl: true` the control takes all the room right of the
// label; use it for a SettingsField or SettingsCombo.
ItemDelegate {
    id: root

    property string iconName: ""
    property string value: ""
    property bool showChevron: true
    property bool fillControl: false
    property bool showSeparator: true          // set by SettingsSection
    property alias control: controlSlot.data

    readonly property real _labelX: leftPadding + (iconName !== "" ? 24 + 14 : 0)

    width: parent ? parent.width : implicitWidth
    implicitHeight: Math.max(54, contentItem.implicitHeight + 20)
    leftPadding: 16
    rightPadding: 16
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: false
    font.pixelSize: Theme.body; font.letterSpacing: Theme.tracking(Theme.body)

    contentItem: RowLayout {
        spacing: 14
        // Dim while pressed; the card's rounded corners rule out a fill.
        opacity: root.down ? 0.5 : 1

        IconImage {
            visible: root.iconName !== ""
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            name: root.iconName
            color: Theme.iconColor
            sourceSize: Qt.size(22, 22)
            fillMode: Image.PreserveAspectFit
        }

        Label {
            Layout.fillWidth: !root.fillControl
            Layout.maximumWidth: root.fillControl ? root.availableWidth * 0.45 : -1
            text: root.text
            textFormat: Text.StyledText
            font: root.font
            color: palette.text
            wrapMode: Text.WordWrap
            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
        }

        Label {
            visible: root.value !== ""
            text: root.value
            font: root.font
            color: Theme.secondaryText
        }

        Item {
            id: controlSlot
            visible: children.length > 0
            Layout.fillWidth: root.fillControl
            Layout.fillHeight: root.fillControl
            implicitWidth: children.length > 0 ? children[0].implicitWidth : 0
            implicitHeight: children.length > 0 ? children[0].implicitHeight : 0
        }

        Label {
            visible: root.showChevron
            text: "›"
            font.pixelSize: Theme.title2
            color: Theme.secondaryText
        }
    }

    // Hairline under the row, starting at the label as on iOS.
    background: Item {
        Rectangle {
            visible: root.showSeparator
            x: root._labelX
            width: parent.width - x - root.rightPadding
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.separator
        }
    }
}

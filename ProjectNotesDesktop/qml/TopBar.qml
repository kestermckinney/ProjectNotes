// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Breadcrumb / actions bar above the main content region.
Rectangle {
    id: bar
    color: Theme.bg
    implicitHeight: 52

    property string crumbIcon: "description"
    property string crumbTitle: "Projects"
    property string crumbSub: ""
    property string newLabel: "New"
    property bool showNew: true
    property bool showSearch: true
    property bool showBack: false
    property bool showExport: false
    property bool showFilter: true

    signal searchEdited(string text)
    signal addClicked()
    signal backClicked()
    signal exportClicked()
    signal filterClicked()

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width; height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 16
        spacing: 12

        // Back button (shown when navigated into a detail page)
        Rectangle {
            visible: bar.showBack
            implicitWidth: 30; implicitHeight: 30; radius: Theme.radiusSm
            color: backHover.hovered ? Theme.surface2 : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "arrow_back"; size: 18; color: Theme.text2 }
            HoverHandler { id: backHover }
            TapHandler { onTapped: bar.backClicked() }
        }

        MaterialIcon { name: bar.crumbIcon; size: 18; color: Theme.text3 }
        Text {
            text: bar.crumbTitle
            color: Theme.text
            font.pixelSize: 15
            font.weight: Font.DemiBold
        }
        MaterialIcon {
            name: "chevron_right"; size: 18; color: Theme.text3
            visible: bar.crumbSub !== ""
        }
        Text {
            text: bar.crumbSub
            color: Theme.text2
            font.pixelSize: 13
            visible: bar.crumbSub !== ""
        }

        Item { Layout.fillWidth: true }

        // Export XML (shown on record detail pages)
        Rectangle {
            visible: bar.showExport
            implicitHeight: 34
            implicitWidth: expRow.implicitWidth + 22
            radius: Theme.radiusSm
            color: expHover.hovered ? Theme.surface2 : Theme.surface
            border.color: Theme.border
            RowLayout {
                id: expRow
                anchors.centerIn: parent
                spacing: 6
                MaterialIcon { name: "download"; size: 17; color: Theme.text2 }
                Text { text: qsTr("Export XML"); color: Theme.text; font.pixelSize: 13 }
            }
            HoverHandler { id: expHover }
            TapHandler { onTapped: bar.exportClicked() }
        }

        // Global search box
        Rectangle {
            visible: bar.showSearch
            Layout.preferredWidth: 260
            implicitHeight: 34
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: Theme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 8
                MaterialIcon { name: "search"; size: 18; color: Theme.text3 }
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "Search entire database"
                    color: Theme.text
                    placeholderTextColor: Theme.text3
                    background: null
                    font.pixelSize: 13
                    onTextEdited: bar.searchEdited(text)
                }
                Text { text: "⌘K"; color: Theme.text3; font.pixelSize: 11 }
            }
        }

        // Filter button (opens the filter editor for the current list)
        Rectangle {
            visible: bar.showFilter
            implicitHeight: 34
            implicitWidth: filtRow.implicitWidth + 22
            radius: Theme.radiusSm
            color: filtHover.hovered ? Theme.surface2 : Theme.surface
            border.color: Theme.border
            RowLayout {
                id: filtRow
                anchors.centerIn: parent
                spacing: 6
                MaterialIcon { name: "filter_list"; size: 17; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
                Text { text: qsTr("Filter"); color: Theme.text; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter }
            }
            HoverHandler { id: filtHover }
            TapHandler { onTapped: bar.filterClicked() }
        }

        // Add button
        Button {
            visible: bar.showNew
            implicitHeight: 34
            padding: 10
            background: Rectangle {
                radius: Theme.radiusSm
                color: parent.down ? Theme.accentStrong : Theme.accent
            }
            contentItem: RowLayout {
                spacing: 6
                MaterialIcon { name: "add"; size: 18; color: "#ffffff"; Layout.alignment: Qt.AlignVCenter }
                Text {
                    text: bar.newLabel; color: "#ffffff"; font.pixelSize: 13; font.weight: Font.DemiBold
                    verticalAlignment: Text.AlignVCenter; Layout.alignment: Qt.AlignVCenter
                }
            }
            onClicked: bar.addClicked()
        }
    }
}

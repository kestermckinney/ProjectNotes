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
    property bool showDelete: false
    property bool showFilter: true
    // True when the current section's model has any active column filter
    // (manual or Quick Filter) — highlights the Filter button so it's obvious
    // a filter is narrowing the list. Computed by the caller (Main.qml), same
    // as every other bound property here — this component stays presentational.
    property bool filterActive: false
    property bool showSort: true
    // True when the current section's model has an active sort — highlights
    // the Sort button the same way filterActive highlights Filter.
    property bool sortActive: false
    // The Sort button itself, so the caller can position SortMenu.openFor()
    // beside it (SortMenu is section-driven and opened directly by the
    // caller, unlike Filter which routes through this bar's filterClicked
    // signal — see Main.qml's onSortClicked wiring).
    property alias sortButtonItem: sortBtn

    // Set by the caller to resync the field's display to whichever section's
    // quick search is currently active (e.g. on rail section switch). Reading
    // it back gets the live text, but callers should treat it as write-only —
    // it's an alias, not a binding, so it won't track edits made elsewhere.
    property alias searchText: searchField.text

    signal searchEdited(string text)
    signal addClicked()
    signal backClicked()
    signal exportClicked()
    signal deleteClicked()
    signal filterClicked()
    signal sortClicked()

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

        // Current UI zoom (Ctrl+wheel / Ctrl +/- / app menu). Always present so
        // the level is never a mystery, but kept to a bare percentage — no
        // +/- controls duplicated here — so it stays a compact readout, not
        // another toolbar. Click opens a preset picker.
        Rectangle {
            id: zoomIndicator
            implicitHeight: 28
            implicitWidth: zoomRow.implicitWidth + 14
            radius: Theme.radiusSm
            color: (zoomHover.hovered || zoomMenu.visible) ? Theme.surface2 : "transparent"
            RowLayout {
                id: zoomRow
                anchors.centerIn: parent
                spacing: 4
                MaterialIcon { name: "zoom_in"; size: 14; color: Theme.text3 }
                Text {
                    text: Math.round(Theme.uiScale * 100) + "%"
                    color: Theme.text2
                    font.pixelSize: 12
                }
            }
            HoverHandler { id: zoomHover }
            TapHandler {
                gesturePolicy: TapHandler.ReleaseWithinBounds
                onTapped: {
                    var p = zoomIndicator.mapToItem(null, 0, zoomIndicator.height)
                    zoomMenu.openAt(p.x, p.y)
                }
            }
            ToolTip.visible: zoomHover.hovered && !zoomMenu.visible
            ToolTip.text: qsTr("Zoom Level")
            ToolTip.delay: 400
        }

        // Preset zoom levels, opened from the indicator above.
        Popup {
            id: zoomMenu
            readonly property var levels: [130, 120, 110, 100, 90, 80]

            modal: true
            dim: false
            padding: 5
            width: _zoomContent.implicitWidth + leftPadding + rightPadding
            height: _zoomContent.implicitHeight + topPadding + bottomPadding
            // Deliberately an in-scene popup (see RecordContextMenu) — popupType:
            // Popup.Window forwards clicks through to whatever sits behind it.
            parent: Overlay.overlay
            scale: Theme.uiScale
            transformOrigin: Item.TopLeft

            background: Rectangle {
                radius: Theme.radius
                color: Theme.surface
                border.color: Theme.border
            }

            // Open just under the indicator, at scene (overlay) coordinates.
            function openAt(sx, sy) {
                var sw = width * Theme.uiScale
                x = Math.max(6, Math.min(sx, (parent ? parent.width : sx + sw) - sw - 6))
                y = sy
                open()
            }

            contentItem: ColumnLayout {
                id: _zoomContent
                spacing: 0
                Repeater {
                    model: zoomMenu.levels
                    delegate: Rectangle {
                        id: levelRow
                        required property int modelData
                        readonly property bool current: Math.round(Theme.uiScale * 100) === modelData
                        Layout.fillWidth: true
                        implicitWidth: levelContent.implicitWidth + 20
                        implicitHeight: 30
                        radius: Theme.radiusSm
                        color: levelHover.hovered ? Theme.surface2 : "transparent"
                        RowLayout {
                            id: levelContent
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            spacing: 10
                            Text {
                                text: levelRow.modelData + "%"
                                color: Theme.text
                                font.pixelSize: 13
                                font.weight: levelRow.current ? Font.DemiBold : Font.Normal
                                Layout.fillWidth: true
                            }
                            MaterialIcon {
                                visible: levelRow.current
                                name: "check"; size: 15; color: Theme.accent
                            }
                        }
                        HoverHandler { id: levelHover }
                        TapHandler {
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: {
                                Theme.uiScale = levelRow.modelData / 100
                                zoomMenu.close()
                            }
                        }
                    }
                }
            }
        }

        // Delete record (shown on record detail pages)
        Rectangle {
            visible: bar.showDelete
            implicitHeight: 34
            implicitWidth: delRow.implicitWidth + 22
            radius: Theme.radiusSm
            color: delHover.hovered ? Theme.redSoft : Theme.surface
            border.color: delHover.hovered ? Theme.red : Theme.border
            RowLayout {
                id: delRow
                anchors.centerIn: parent
                spacing: 6
                MaterialIcon { name: "delete"; size: 17; color: Theme.red }
                Text { text: qsTr("Delete"); color: Theme.red; font.pixelSize: 13 }
            }
            HoverHandler { id: delHover }
            TapHandler { onTapped: bar.deleteClicked() }
        }

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

        // Filter button (opens the filter editor for the current list) —
        // highlighted (accent bg/border/icon) while filterActive is true.
        Rectangle {
            visible: bar.showFilter
            implicitHeight: 34
            implicitWidth: filtRow.implicitWidth + 22
            radius: Theme.radiusSm
            color: bar.filterActive ? Theme.accentSoft : (filtHover.hovered ? Theme.surface2 : Theme.surface)
            border.color: bar.filterActive ? Theme.accent : Theme.border
            RowLayout {
                id: filtRow
                anchors.centerIn: parent
                spacing: 6
                MaterialIcon { name: "filter_list"; size: 17; color: bar.filterActive ? Theme.accent : Theme.text2; Layout.alignment: Qt.AlignVCenter }
                Text { text: qsTr("Filter"); color: bar.filterActive ? Theme.accent : Theme.text; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter }
            }
            HoverHandler { id: filtHover }
            TapHandler { onTapped: bar.filterClicked() }
        }

        // Sort button (opens SortMenu for the current list) — same highlighted
        // treatment as Filter while sortActive is true.
        Rectangle {
            id: sortBtn
            visible: bar.showSort
            implicitHeight: 34
            implicitWidth: sortRow.implicitWidth + 22
            radius: Theme.radiusSm
            color: bar.sortActive ? Theme.accentSoft : (sortHover.hovered ? Theme.surface2 : Theme.surface)
            border.color: bar.sortActive ? Theme.accent : Theme.border
            RowLayout {
                id: sortRow
                anchors.centerIn: parent
                spacing: 6
                MaterialIcon { name: "swap_vert"; size: 17; color: bar.sortActive ? Theme.accent : Theme.text2; Layout.alignment: Qt.AlignVCenter }
                Text { text: qsTr("Sort"); color: bar.sortActive ? Theme.accent : Theme.text; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter }
            }
            HoverHandler { id: sortHover }
            TapHandler { onTapped: bar.sortClicked() }
        }

        // Add button
        Button {
            visible: bar.showNew
            implicitHeight: 34
            leftPadding: 12; rightPadding: 12; topPadding: 0; bottomPadding: 0
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

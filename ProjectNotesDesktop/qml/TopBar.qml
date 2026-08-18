// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Breadcrumb / actions bar above the main content region.
Rectangle {
    id: bar
    color: Theme.bg
    implicitHeight: 44

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

    // Puts the keyboard cursor in the quick search field and selects whatever is
    // already typed, so the next keystroke replaces it (browser address-bar
    // behaviour). Wired to AppShortcuts' "focus_quick_search" key in Main.qml;
    // a no-op on the sections and detail pages that don't show the field.
    function focusSearch() {
        if (!bar.showSearch)
            return
        searchField.forceActiveFocus()
        searchField.selectAll()
    }

    // Clears the field and tells the caller the search is off. Used by the
    // clear button and by Esc while the field has focus; clear() is programmatic
    // so TextField.onTextEdited never fires for it — hence the explicit signal.
    function clearSearch() {
        if (searchField.text === "")
            return
        searchField.clear()
        bar.searchEdited("")
    }

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
        anchors.leftMargin: 14
        anchors.rightMargin: 12
        spacing: 9

        // Back button (shown when navigated into a detail page)
        Rectangle {
            visible: bar.showBack
            implicitWidth: 26; implicitHeight: 26; radius: Theme.radiusSm
            color: backHover.hovered ? Theme.surface2 : "transparent"
            MaterialIcon { anchors.centerIn: parent; name: "arrow_back"; size: 16; color: Theme.text2 }
            HoverHandler { id: backHover }
            TapHandler { onTapped: bar.backClicked() }
        }

        MaterialIcon { name: bar.crumbIcon; size: 16; color: Theme.text3 }
        Text {
            text: bar.crumbTitle
            color: Theme.text
            font.pixelSize: Theme.fontXl
            font.weight: Font.DemiBold
        }
        MaterialIcon {
            name: "chevron_right"; size: 16; color: Theme.text3
            visible: bar.crumbSub !== ""
        }
        Text {
            text: bar.crumbSub
            color: Theme.text2
            font.pixelSize: Theme.fontBody
            visible: bar.crumbSub !== ""
        }

        Item { Layout.fillWidth: true }

        // Current UI zoom (Ctrl+wheel / Ctrl +/- / app menu). Always present so
        // the level is never a mystery, but kept to a bare percentage — no
        // +/- controls duplicated here — so it stays a compact readout, not
        // another toolbar. Click opens a preset picker.
        Rectangle {
            id: zoomIndicator
            implicitHeight: 24
            implicitWidth: zoomRow.implicitWidth + 12
            radius: Theme.radiusSm
            color: (zoomHover.hovered || zoomMenu.visible) ? Theme.surface2 : "transparent"
            RowLayout {
                id: zoomRow
                anchors.centerIn: parent
                spacing: 3
                MaterialIcon { name: "zoom_in"; size: 12; color: Theme.text3 }
                Text {
                    text: Math.round(Theme.uiScale * 100) + "%"
                    color: Theme.text2
                    font.pixelSize: Theme.fontSm
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
            padding: 4
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

            // Clicking away dismisses the menu and nothing else — see ClickShield.qml.
            ClickShield { host: zoomMenu }

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
                        implicitWidth: levelContent.implicitWidth + 16
                        implicitHeight: Theme.menuRowHeight
                        radius: Theme.radiusSm
                        color: levelHover.hovered ? Theme.surface2 : "transparent"
                        RowLayout {
                            id: levelContent
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 8
                            Text {
                                text: levelRow.modelData + "%"
                                color: Theme.text
                                font.pixelSize: Theme.menuFont
                                font.weight: levelRow.current ? Font.DemiBold : Font.Normal
                                Layout.fillWidth: true
                            }
                            MaterialIcon {
                                visible: levelRow.current
                                name: "check"; size: Theme.menuCheckSize; color: Theme.accent
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
            implicitHeight: 28
            implicitWidth: delRow.implicitWidth + 18
            radius: Theme.radiusSm
            color: delHover.hovered ? Theme.redSoft : Theme.surface
            border.color: delHover.hovered ? Theme.red : Theme.border
            RowLayout {
                id: delRow
                anchors.centerIn: parent
                spacing: 5
                MaterialIcon { name: "delete"; size: 15; color: Theme.red }
                Text { text: qsTr("Delete"); color: Theme.red; font.pixelSize: Theme.fontBody }
            }
            HoverHandler { id: delHover }
            TapHandler { onTapped: bar.deleteClicked() }
        }

        // Export XML (shown on record detail pages)
        Rectangle {
            visible: bar.showExport
            implicitHeight: 28
            implicitWidth: expRow.implicitWidth + 18
            radius: Theme.radiusSm
            color: expHover.hovered ? Theme.surface2 : Theme.surface
            border.color: Theme.border
            RowLayout {
                id: expRow
                anchors.centerIn: parent
                spacing: 5
                MaterialIcon { name: "download"; size: 15; color: Theme.text2 }
                Text { text: qsTr("Export XML"); color: Theme.text; font.pixelSize: Theme.fontBody }
            }
            HoverHandler { id: expHover }
            TapHandler { onTapped: bar.exportClicked() }
        }

        // Global search box
        Rectangle {
            id: searchBox
            visible: bar.showSearch
            Layout.preferredWidth: 220
            implicitHeight: 28
            radius: Theme.radiusSm
            color: Theme.surface
            // Accent outline while focused, so the focus-search shortcut visibly
            // lands somewhere (same treatment as SearchPage's field).
            border.color: searchField.activeFocus ? Theme.accent : Theme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 9
                anchors.rightMargin: 7
                spacing: 6
                MaterialIcon { name: "search"; size: 16; color: Theme.text3 }
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    // "Quick Page Search", not "search the database" — this
                    // field filters the list on the page you're looking at
                    // (setQuickSearch on the section's model). Searching the
                    // whole database is the Search section.
                    placeholderText: qsTr("Quick Page Search")
                    color: Theme.text
                    placeholderTextColor: Theme.text3
                    background: null
                    font.pixelSize: Theme.fontBody
                    onTextEdited: bar.searchEdited(text)
                    // Esc clears the field while it has focus. With nothing to
                    // clear the key falls through to whatever else wants it
                    // (Main.qml's full-screen exit).
                    Keys.onEscapePressed: (ev) => {
                        if (searchField.text === "") {
                            ev.accepted = false
                            return
                        }
                        bar.clearSearch()
                    }
                }
                // Clear button — appears only once there's something to clear.
                // Replaces the "⌘K" hint that used to sit here: that key opens
                // the full search page rather than focusing this field, and the
                // glyph was the hardcoded macOS one on every platform. The
                // field's real shortcut is on the tooltip below, rendered per-OS.
                MaterialIcon {
                    name: "cancel"
                    size: 15
                    color: clearHover.hovered ? Theme.text2 : Theme.text3
                    visible: searchField.text !== ""
                    Layout.alignment: Qt.AlignVCenter
                    HoverHandler { id: clearHover }
                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                        onTapped: bar.clearSearch()
                    }
                    ToolTip.visible: clearHover.hovered
                    ToolTip.text: qsTr("Clear Search")
                    ToolTip.delay: 400
                }
            }
            HoverHandler { id: searchHover }
            // Only while the field is idle — no tooltip hovering over the text
            // someone is in the middle of typing.
            ToolTip.visible: searchHover.hovered && !searchField.activeFocus && !clearHover.hovered
            ToolTip.text: qsTr("Quick Page Search (%1)").arg(AppShortcuts.text("focus_quick_search"))
            ToolTip.delay: 400
        }

        // Filter button (opens the filter editor for the current list) —
        // highlighted (accent bg/border/icon) while filterActive is true.
        Rectangle {
            visible: bar.showFilter
            implicitHeight: 28
            implicitWidth: filtRow.implicitWidth + 18
            radius: Theme.radiusSm
            color: bar.filterActive ? Theme.accentSoft : (filtHover.hovered ? Theme.surface2 : Theme.surface)
            border.color: bar.filterActive ? Theme.accent : Theme.border
            RowLayout {
                id: filtRow
                anchors.centerIn: parent
                spacing: 5
                MaterialIcon { name: "filter_list"; size: 15; color: bar.filterActive ? Theme.accent : Theme.text2; Layout.alignment: Qt.AlignVCenter }
                Text { text: qsTr("Filter"); color: bar.filterActive ? Theme.accent : Theme.text; font.pixelSize: Theme.fontBody; verticalAlignment: Text.AlignVCenter }
            }
            HoverHandler { id: filtHover }
            TapHandler { onTapped: bar.filterClicked() }
        }

        // Sort button (opens SortMenu for the current list) — same highlighted
        // treatment as Filter while sortActive is true.
        Rectangle {
            id: sortBtn
            visible: bar.showSort
            implicitHeight: 28
            implicitWidth: sortRow.implicitWidth + 18
            radius: Theme.radiusSm
            color: bar.sortActive ? Theme.accentSoft : (sortHover.hovered ? Theme.surface2 : Theme.surface)
            border.color: bar.sortActive ? Theme.accent : Theme.border
            RowLayout {
                id: sortRow
                anchors.centerIn: parent
                spacing: 5
                MaterialIcon { name: "swap_vert"; size: 15; color: bar.sortActive ? Theme.accent : Theme.text2; Layout.alignment: Qt.AlignVCenter }
                Text { text: qsTr("Sort"); color: bar.sortActive ? Theme.accent : Theme.text; font.pixelSize: Theme.fontBody; verticalAlignment: Text.AlignVCenter }
            }
            HoverHandler { id: sortHover }
            TapHandler { onTapped: bar.sortClicked() }
        }

        // Add button
        Button {
            visible: bar.showNew
            primary: true
            implicitHeight: 28
            leftPadding: 10; rightPadding: 10; topPadding: 0; bottomPadding: 0
            contentItem: RowLayout {
                spacing: 5
                MaterialIcon { name: "add"; size: 16; color: "#ffffff"; Layout.alignment: Qt.AlignVCenter }
                Text {
                    text: bar.newLabel; color: "#ffffff"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                    verticalAlignment: Text.AlignVCenter; Layout.alignment: Qt.AlignVCenter
                }
            }
            onClicked: bar.addClicked()
        }
    }
}

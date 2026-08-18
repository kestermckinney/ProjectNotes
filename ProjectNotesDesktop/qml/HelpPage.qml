// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// User Guide — an embedded page shown in the shell's content area as the "help"
// section (see Main.qml selectSection / sectionMeta), replacing the main content
// like the Settings page rather than floating in a separate window.
//
// Renders the same Markdown that builds the mkdocs website (bundled under
// qrc:/…/help/ and served by HelpController) with a collapsible topic tree,
// full-text search, in-topic link navigation with back/forward history, and
// context-sensitive opening. Styled entirely from Theme.qml.
Rectangle {
    id: page
    color: Theme.bg

    // Topic the shell wants shown; also re-drivable via openHelp() while embedded.
    property string topic: "index.md"

    // Canonical path (e.g. "InterfaceOverview/ProjectPage.md") of the shown topic.
    property string current: "index.md"

    // Navigation history for the in-topic links.
    property var _back: []
    property var _fwd: []

    // TOC model + per-section expand state, parsed once from HelpController.
    property var sections: []
    property var expanded: ({})

    // Live search state.
    property string query: ""
    property var results: []

    // ── Public API ──────────────────────────────────────────────────────────────
    function openHelp(topicPath) {
        _ensureToc()
        var p = (topicPath && topicPath.length) ? topicPath : "index.md"
        page.query = ""
        page._back = []
        page._fwd = []
        _expandFor(p)
        page.current = p
        contentFlick.contentY = 0
    }

    // ── Navigation ───────────────────────────────────────────────────────────────
    function navigate(path) {
        if (!path || path === page.current)
            return
        if (page.current) {
            var b = page._back.slice(); b.push(page.current); page._back = b
        }
        page._fwd = []
        page.query = ""
        _expandFor(path)
        page.current = path
        contentFlick.contentY = 0
    }

    function goBack() {
        if (!page._back.length) return
        var b = page._back.slice(); var p = b.pop(); page._back = b
        var f = page._fwd.slice(); f.push(page.current); page._fwd = f
        _expandFor(p); page.current = p; contentFlick.contentY = 0
    }

    function goForward() {
        if (!page._fwd.length) return
        var f = page._fwd.slice(); var p = f.pop(); page._fwd = f
        var b = page._back.slice(); b.push(page.current); page._back = b
        _expandFor(p); page.current = p; contentFlick.contentY = 0
    }

    // ── Helpers ──────────────────────────────────────────────────────────────────
    function _ensureToc() {
        if (page.sections.length > 0) return
        try {
            var o = JSON.parse(HelpController.tocJson())
            page.sections = o.sections || []
        } catch (e) {
            page.sections = []
        }
    }

    function _expandFor(path) {
        var e = {}
        for (var k in page.expanded) e[k] = page.expanded[k]
        for (var i = 0; i < page.sections.length; i++) {
            var s = page.sections[i]
            for (var j = 0; j < s.items.length; j++)
                if (s.items[j].path === path) e[s.title] = true
        }
        page.expanded = e
    }

    function _toggleSection(title) {
        var e = {}
        for (var k in page.expanded) e[k] = page.expanded[k]
        e[title] = !e[title]
        page.expanded = e
    }

    onQueryChanged: results = query.trim().length ? HelpController.search(query) : []

    Component.onCompleted: { _ensureToc(); openHelp(page.topic) }

    // ── Layout ───────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header: back / forward + current topic title.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 40
            color: Theme.surface

            Rectangle {                       // bottom divider
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: Theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 9; anchors.rightMargin: 12
                spacing: 5

                NavButton {
                    glyph: "arrow_back"
                    enabled: page._back.length > 0
                    onClicked: page.goBack()
                }
                NavButton {
                    glyph: "arrow_forward"
                    enabled: page._fwd.length > 0
                    onClicked: page.goForward()
                }

                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 3
                    text: HelpController.topicTitle(page.current)
                    color: Theme.text
                    font.pixelSize: Theme.fontXl
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
            }
        }

        // Body: topic pane (left) + content (right).
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Left pane: search + TOC / results ────────────────────────────────
            Rectangle {
                Layout.preferredWidth: 252
                Layout.fillHeight: true
                color: Theme.sidebar

                Rectangle {                    // right divider
                    anchors.right: parent.right
                    height: parent.height; width: 1
                    color: Theme.border
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Search box
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.margins: 9
                        implicitHeight: 30
                        radius: Theme.radiusSm
                        color: Theme.surface
                        border.color: searchInput.activeFocus ? Theme.accent : Theme.border
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 5
                            spacing: 5

                            MaterialIcon { name: "search"; size: 14; color: Theme.text3 }

                            TextField {
                                id: searchInput
                                Layout.fillWidth: true
                                placeholderText: qsTr("Search help…")
                                text: page.query
                                onTextChanged: page.query = text
                                color: Theme.text
                                placeholderTextColor: Theme.text3
                                background: null
                                font.pixelSize: Theme.fontBody
                                selectByMouse: true
                            }

                            MaterialIcon {
                                name: "close"; size: 14; color: Theme.text3
                                visible: page.query.length > 0
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: { page.query = ""; searchInput.text = "" }
                                }
                            }
                        }
                    }

                    // Scrollable topic list — either search results or the TOC tree.
                    Flickable {
                        id: navFlick
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: width
                        contentHeight: navCol.implicitHeight
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        ColumnLayout {
                            id: navCol
                            width: navFlick.width
                            spacing: 0

                            // ── Search results ───────────────────────────────────
                            Text {
                                visible: page.query.length > 0 && page.results.length === 0
                                text: qsTr("No results for “%1”.").arg(page.query)
                                color: Theme.text3
                                font.pixelSize: Theme.fontSm
                                Layout.margins: 10
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            Repeater {
                                model: page.query.length > 0 ? page.results : []
                                delegate: Rectangle {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 5; Layout.rightMargin: 5
                                    implicitHeight: resCol.implicitHeight + 10
                                    radius: Theme.radiusSm
                                    color: resHover.hovered ? Theme.surface2 : "transparent"

                                    ColumnLayout {
                                        id: resCol
                                        anchors.left: parent.left; anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: 8; anchors.rightMargin: 8
                                        spacing: 1
                                        Text {
                                            text: modelData.title
                                            color: Theme.text
                                            font.pixelSize: Theme.fontBody
                                            font.weight: Font.DemiBold
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            visible: (modelData.snippet || "").length > 0
                                            text: modelData.snippet
                                            color: Theme.text3
                                            font.pixelSize: Theme.fontXs
                                            Layout.fillWidth: true
                                            wrapMode: Text.WordWrap
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                        }
                                    }
                                    HoverHandler { id: resHover }
                                    TapHandler { onTapped: page.navigate(modelData.path) }
                                }
                            }

                            // ── TOC tree ─────────────────────────────────────────
                            Repeater {
                                model: page.query.length > 0 ? [] : page.sections
                                delegate: ColumnLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 0

                                    // Section header (collapsible)
                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: 30
                                        color: secHover.hovered ? Theme.surface2 : "transparent"
                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 7; anchors.rightMargin: 8
                                            spacing: 3
                                            MaterialIcon {
                                                name: page.expanded[modelData.title] ? "expand_more" : "chevron_right"
                                                size: 16; color: Theme.text2
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.title.toUpperCase()
                                                color: Theme.text2
                                                font.pixelSize: Theme.fontXs
                                                font.weight: Font.Bold
                                                elide: Text.ElideRight
                                            }
                                        }
                                        HoverHandler { id: secHover }
                                        TapHandler { onTapped: page._toggleSection(modelData.title) }
                                    }

                                    // Section items
                                    Repeater {
                                        model: page.expanded[modelData.title] ? modelData.items : []
                                        delegate: Rectangle {
                                            required property var modelData
                                            readonly property bool isCurrent: modelData.path === page.current
                                            Layout.fillWidth: true
                                            implicitHeight: 27
                                            color: isCurrent ? Theme.accentSoft
                                                             : (itemHover.hovered ? Theme.surface2 : "transparent")
                                            Rectangle {           // active accent bar
                                                anchors.left: parent.left
                                                width: 3; height: parent.height
                                                color: isCurrent ? Theme.accent : "transparent"
                                            }
                                            Text {
                                                anchors.fill: parent
                                                anchors.leftMargin: 26; anchors.rightMargin: 8
                                                verticalAlignment: Text.AlignVCenter
                                                text: modelData.title
                                                color: isCurrent ? Theme.accentStrong : Theme.text
                                                font.pixelSize: Theme.fontBody
                                                font.weight: isCurrent ? Font.DemiBold : Font.Normal
                                                elide: Text.ElideRight
                                            }
                                            HoverHandler { id: itemHover }
                                            TapHandler { onTapped: page.navigate(modelData.path) }
                                        }
                                    }
                                }
                            }

                            Item { Layout.preferredHeight: 8 }   // bottom padding
                        }
                    }
                }
            }

            // ── Content pane ──────────────────────────────────────────────────────
            Flickable {
                id: contentFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: docText.implicitHeight + 40
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                // A read-only TextEdit rather than a plain Text: Text has no
                // textDocument property (that's exclusive to TextEdit/TextArea),
                // so applySpacing() below would silently receive undefined and
                // do nothing. TextEdit renders Markdown identically and adds
                // free-of-charge text selection/copy.
                TextEdit {
                    id: docText
                    x: 24; y: 20
                    width: contentFlick.width - 48
                    text: HelpController.topicMarkdown(page.current)
                    textFormat: TextEdit.MarkdownText
                    wrapMode: TextEdit.WordWrap
                    color: Theme.text
                    font.pixelSize: Theme.fontLg
                    readOnly: true
                    activeFocusOnPress: false
                    selectByMouse: true
                    // Qt's Markdown-to-QTextDocument import packs blocks tight
                    // enough to look cramped in-app; widen the live document's
                    // margins/line height once it has (re-)parsed. Deferred a
                    // tick since the document isn't repopulated synchronously
                    // with the text change — by which time docText may itself
                    // be gone (e.g. navigated away from Help before the tick
                    // fires), so guard against the now-null id.
                    onTextChanged: Qt.callLater(function() {
                        if (docText) HelpController.applySpacing(docText.textDocument)
                    })
                    onLinkActivated: (link) => {
                        var t = HelpController.resolveLink(page.current, link)
                        if (t && t.length) page.navigate(t)
                        else Qt.openUrlExternally(link)
                    }
                }
            }
        }
    }

    // Small header nav button (back / forward). Uses the built-in Item `enabled`,
    // which also disables the child hover/tap handlers when greyed out.
    component NavButton: Rectangle {
        id: btn
        property string glyph: ""
        signal clicked()
        implicitWidth: 26; implicitHeight: 26
        radius: Theme.radiusSm
        color: btn.enabled && btnHover.hovered ? Theme.surface2 : "transparent"
        opacity: btn.enabled ? 1.0 : 0.35
        MaterialIcon {
            anchors.centerIn: parent
            name: btn.glyph; size: 17; color: Theme.text2
        }
        HoverHandler { id: btnHover }
        TapHandler { onTapped: btn.clicked() }
    }
}

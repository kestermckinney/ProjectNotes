// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Formatting toolbar bound to a target TextArea/TextEdit (`editor`). Each button
// applies a TextFormatter operation to the editor's QTextDocument over its
// current selection, then restores focus + selection so the highlight persists.
Rectangle {
    id: bar
    property var editor: null
    // Shared full-field spell-check dialog + the editor's SpellCheck object, so
    // the spellcheck button can run a whole-field check on `editor`.
    property var dialog: null
    property var spell: null

    // Selection range captured the moment a picker popup opens. Opening a popup
    // steals focus and drops the editor's live selection, so we stash it here and
    // apply formatting against these values.
    property int _selStart: 0
    property int _selEnd: 0

    implicitHeight: 34
    radius: Theme.radiusSm
    color: Theme.surface2
    border.color: Theme.border

    readonly property var _fontSizes: [8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 32, 36, 48, 72]

    // Paragraph styles for the Style dropdown. `style` maps to TextFormatter.applyStyle:
    // 0 = normal body text, 9..14 = Heading 1..6.
    readonly property var _paragraphStyles: [
        { label: qsTr("Normal text"), style: 0,  px: 12, wt: Font.Normal   },
        { label: qsTr("Heading 1"),   style: 9,  px: 19, wt: Font.Bold     },
        { label: qsTr("Heading 2"),   style: 10, px: 16, wt: Font.Bold     },
        { label: qsTr("Heading 3"),   style: 11, px: 14, wt: Font.DemiBold },
        { label: qsTr("Heading 4"),   style: 12, px: 12, wt: Font.DemiBold },
        { label: qsTr("Heading 5"),   style: 13, px: 12, wt: Font.DemiBold },
        { label: qsTr("Heading 6"),   style: 14, px: 11, wt: Font.DemiBold }
    ]

    // Google-Docs-style palette: grayscale row + hue columns at several shades.
    readonly property var _fontColors: [
        "#000000", "#434343", "#666666", "#999999", "#b7b7b7", "#cccccc", "#d9d9d9", "#ffffff",
        "#980000", "#ff0000", "#ff9900", "#ffff00", "#00ff00", "#00ffff", "#4a86e8", "#0000ff",
        "#9900ff", "#ff00ff", "#e6b8af", "#f4cccc", "#fce5cd", "#fff2cc", "#d9ead3", "#d0e0e3",
        "#c9daf8", "#cfe2f3", "#d9d2e9", "#ead1dc", "#dd7e6b", "#ea9999", "#f9cb9c", "#ffe599",
        "#b6d7a8", "#a2c4c9", "#a4c2f4", "#9fc5e8", "#b4a7d6", "#d5a6bd", "#cc4125", "#e06666",
        "#f6b26b", "#ffd966", "#93c47d", "#76a5af", "#6d9eeb", "#6fa8dc", "#8e7cc3", "#c27ba0"
    ]
    readonly property var _highlightColors: [
        "#ffff00", "#ffd966", "#f6b26b", "#f4cccc", "#ea9999", "#ffe599", "#fff2cc", "#fce5cd",
        "#d9ead3", "#b6d7a8", "#93c47d", "#a2c4c9", "#d0e0e3", "#a4c2f4", "#9fc5e8", "#cfe2f3",
        "#b4a7d6", "#d9d2e9", "#d5a6bd", "#ead1dc", "#00ff00", "#00ffff", "#ff00ff", "#ff9900"
    ]

    // Live snapshot of the format under the cursor (or spanning the current
    // selection). Drives the toolbar's font/size/attribute/color/style/alignment
    // indicators so they always reflect what's actually under the cursor.
    //
    // Deliberately NOT a plain property binding on editor.selectionStart/End:
    // those signals can fire WHILE the document is still being mutated (e.g.
    // noteEdit.text = ... reparsing HTML into the QTextDocument block-by-block
    // when switching notes). Querying the document with a fresh QTextCursor
    // reentrantly from inside that same call stack crashed the app. Refreshing
    // via Qt.callLater() defers the read to the next event-loop turn, after
    // the mutation that triggered the signal has fully unwound.
    property var _liveFmt: null

    function _refreshLiveFmt() {
        if (!bar.editor || !bar.editor.textDocument) { bar._liveFmt = null; return }
        var doc = bar.editor.textDocument
        var s = bar.editor.selectionStart
        var e = bar.editor.selectionEnd
        bar._liveFmt = {
            bold: TextFormatter.isBoldAt(doc, s, e),
            italic: TextFormatter.isItalicAt(doc, s, e),
            underline: TextFormatter.isUnderlineAt(doc, s, e),
            strikethrough: TextFormatter.isStrikethroughAt(doc, s, e),
            family: TextFormatter.currentFontFamily(doc, e),
            size: TextFormatter.currentFontPointSize(doc, e),
            color: TextFormatter.currentFontColor(doc, e),
            alignment: TextFormatter.currentAlignment(doc, e),
            style: TextFormatter.currentParagraphStyle(doc, e)
        }
    }

    onEditorChanged: Qt.callLater(_refreshLiveFmt)
    Component.onCompleted: Qt.callLater(_refreshLiveFmt)

    Connections {
        target: bar.editor
        ignoreUnknownSignals: true
        function onSelectionStartChanged() { Qt.callLater(bar._refreshLiveFmt) }
        function onSelectionEndChanged() { Qt.callLater(bar._refreshLiveFmt) }
        function onCursorPositionChanged() { Qt.callLater(bar._refreshLiveFmt) }
        function onTextChanged() { Qt.callLater(bar._refreshLiveFmt) }
    }

    // Immediate ops that use the editor's live selection.
    function _apply(fn) {
        if (!editor) return
        var ss = editor.selectionStart
        var se = editor.selectionEnd
        fn(editor.textDocument, ss, se)
        editor.forceActiveFocus()
        editor.select(ss, se)
        Qt.callLater(bar._refreshLiveFmt)
    }

    // Stash the current selection before a popup opens.
    function _capture() {
        if (!editor) return
        bar._selStart = editor.selectionStart
        bar._selEnd = editor.selectionEnd
    }

    // Ops driven from a popup: apply against the captured selection, then restore.
    function _applyStashed(fn) {
        if (!editor) return
        fn(editor.textDocument, bar._selStart, bar._selEnd)
        editor.forceActiveFocus()
        editor.select(bar._selStart, bar._selEnd)
        Qt.callLater(bar._refreshLiveFmt)
    }

    // Human-readable name for a paragraph-style index (see _paragraphStyles).
    // Falls back to "Style" for indices the detector can't resolve.
    function _styleLabel(idx) {
        for (var i = 0; i < _paragraphStyles.length; i++)
            if (_paragraphStyles[i].style === idx)
                return _paragraphStyles[i].label
        return qsTr("Style")
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        spacing: 1

        component FmtButton: Item {
            id: btn
            property string icon: ""
            property string glyph: ""
            // True when the format this button toggles is active at the
            // current cursor position/selection — shows a highlighted state
            // so the toolbar reflects what's under the cursor.
            property bool active: false
            signal triggered()
            implicitWidth: 26
            implicitHeight: 26
            Layout.alignment: Qt.AlignVCenter
            Rectangle {
                anchors.centerIn: parent
                width: 24; height: 24; radius: Theme.radiusSm
                color: btn.active ? Theme.accentSoft : (hh.hovered ? Theme.surface : "transparent")
                border.color: btn.active ? Theme.accent : "transparent"
                border.width: 1
                MaterialIcon {
                    anchors.centerIn: parent
                    visible: btn.icon !== ""
                    name: btn.icon; size: 15; color: btn.active ? Theme.accent : Theme.text2
                }
                Text {
                    anchors.centerIn: parent
                    visible: btn.glyph !== ""
                    text: btn.glyph
                    color: btn.active ? Theme.accent : Theme.text2
                    font.pixelSize: 13
                    font.weight: Font.Bold
                }
            }
            HoverHandler { id: hh }
            TapHandler { onTapped: btn.triggered() }
        }

        // A labeled dropdown-style chip ("Font ▾", "Size ▾"). Emits clicked().
        component ChipButton: Rectangle {
            id: chip
            property string label: ""
            // Optional: render the label in this font family, so the Font chip
            // previews the typeface under the cursor (e.g. Google-Docs-style).
            property string previewFamily: ""
            property alias contentWidth: chipText.implicitWidth
            signal clicked()
            Layout.alignment: Qt.AlignVCenter
            implicitHeight: 24
            implicitWidth: chipRow.implicitWidth + 11
            radius: Theme.radiusSm
            color: chHover.hovered ? Theme.surface : "transparent"
            RowLayout {
                id: chipRow
                anchors.centerIn: parent
                spacing: 2
                Text {
                    id: chipText
                    text: chip.label
                    color: Theme.text2
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    Layout.maximumWidth: 100
                }
                // Only override the family when we actually have one to preview —
                // font.family is a QString property, so `undefined` can't be assigned
                // to it directly ("Unable to assign [undefined] to QString"; that
                // assignment silently fails and leaves the property at "").  An empty
                // string is not a safe "use the default" value either: Qt's DirectWrite
                // engine treats "" as a real font request, fails to resolve/rasterize
                // it (falls through to the legacy "MS Sans Serif" raster font, which
                // DirectWrite can't use), and exhausts its font fallback list — the
                // next QList::first() on that empty list asserts and crashes the app.
                // A conditional Binding, by contrast, fully detaches when `when` is
                // false and leaves font.family at its normal inherited value.
                Binding {
                    target: chipText
                    property: "font.family"
                    value: chip.previewFamily
                    when: chip.previewFamily.length > 0
                }
                MaterialIcon { name: "arrow_drop_down"; size: 14; color: Theme.text3 }
            }
            HoverHandler { id: chHover }
            TapHandler { onTapped: chip.clicked() }
        }

        component Sep: Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 1; height: 17; color: Theme.border
        }

        FmtButton { icon: "format_bold";        active: bar._liveFmt ? bar._liveFmt.bold : false;          onTriggered: bar._apply(TextFormatter.toggleBold) }
        FmtButton { icon: "format_italic";      active: bar._liveFmt ? bar._liveFmt.italic : false;        onTriggered: bar._apply(TextFormatter.toggleItalic) }
        FmtButton { icon: "format_underlined";  active: bar._liveFmt ? bar._liveFmt.underline : false;     onTriggered: bar._apply(TextFormatter.toggleUnderline) }
        FmtButton { icon: "strikethrough_s";    active: bar._liveFmt ? bar._liveFmt.strikethrough : false; onTriggered: bar._apply(TextFormatter.toggleStrikethrough) }
        Sep {}

        // ── Font family ───────────────────────────────────────────────────────
        // Label + preview always track the font under the cursor.
        ChipButton {
            id: fontChip
            label: bar._liveFmt && bar._liveFmt.family ? bar._liveFmt.family : qsTr("Font")
            previewFamily: bar._liveFmt ? bar._liveFmt.family : ""
            onClicked: { bar._capture(); fontPopup.openFor(fontChip) }
        }

        // ── Font size ─────────────────────────────────────────────────────────
        ChipButton {
            id: sizeChip
            label: bar._liveFmt ? (bar._liveFmt.size + "") : "12"
            onClicked: { bar._capture(); sizePopup.openFor(sizeChip) }
        }
        FmtButton { icon: "text_increase"; onTriggered: bar._apply(TextFormatter.increaseFontSize) }
        FmtButton { icon: "text_decrease"; onTriggered: bar._apply(TextFormatter.decreaseFontSize) }
        Sep {}

        // ── Font color ────────────────────────────────────────────────────────
        FmtButton {
            id: colorBtn
            glyph: "A"
            onTriggered: { bar._capture(); colorPopup.openFor(colorBtn) }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 4
                width: 14; height: 2; radius: 1
                color: bar._liveFmt ? bar._liveFmt.color : Theme.text
            }
        }
        // ── Highlight ─────────────────────────────────────────────────────────
        FmtButton {
            id: hlBtn
            icon: "border_color"
            onTriggered: { bar._capture(); highlightPopup.openFor(hlBtn) }
        }
        Sep {}

        FmtButton { icon: "format_list_bulleted"; onTriggered: bar._apply(TextFormatter.toggleBulletList) }
        FmtButton { icon: "format_indent_increase"; onTriggered: bar._apply(TextFormatter.indentText) }
        FmtButton { icon: "format_indent_decrease"; onTriggered: bar._apply(TextFormatter.unindentText) }
        Sep {}
        FmtButton {
            id: tableBtn
            icon: "grid_on"
            onTriggered: { bar._capture(); tablePopup.openFor(tableBtn) }
        }
        Sep {}
        FmtButton { icon: "format_align_left";    active: bar._liveFmt ? bar._liveFmt.alignment === 0 : false; onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,0) }) }
        FmtButton { icon: "format_align_center";  active: bar._liveFmt ? bar._liveFmt.alignment === 1 : false; onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,1) }) }
        FmtButton { icon: "format_align_right";   active: bar._liveFmt ? bar._liveFmt.alignment === 2 : false; onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,2) }) }
        Sep {}

        // ── Paragraph style (headings) ────────────────────────────────────────
        ChipButton {
            id: styleChip
            label: bar._liveFmt ? bar._styleLabel(bar._liveFmt.style) : qsTr("Style")
            onClicked: { bar._capture(); stylePopup.openFor(styleChip) }
        }

        Item { Layout.fillWidth: true }

        // ── Spell check whole field ───────────────────────────────────────────
        FmtButton {
            icon: "spellcheck"
            visible: bar.dialog !== null && bar.spell !== null
            onTriggered: if (bar.dialog && bar.spell) bar.dialog.openFor(bar.spell, bar.editor)
        }
    }

    // ── Font family popup ─────────────────────────────────────────────────────
    Popup {
        id: fontPopup
        width: 210
        height: 280
        padding: 6
        modal: false
        scale: Theme.uiScale
        transformOrigin: Item.TopLeft
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        // Clicking away dismisses the picker and nothing else — see
        // ClickShield.qml. These pickers are non-modal, so without it the
        // dismissing click also landed in the note body (moving the caret) or on
        // whatever else sat behind them.
        ClickShield { host: fontPopup }

        property var _families: []
        function openFor(anchorItem) {
            _families = TextFormatter.availableFontFamilies()
            fontFilter.text = ""
            var p = anchorItem.mapToItem(bar, 0, anchorItem.height)
            x = Math.max(0, Math.min(p.x, bar.width - width))
            y = p.y + 4
            open()
            fontFilter.forceActiveFocus()
        }
        function _filtered() {
            var q = fontFilter.text.toLowerCase()
            if (q === "") return _families
            return _families.filter(function(f){ return f.toLowerCase().indexOf(q) >= 0 })
        }

        contentItem: ColumnLayout {
            spacing: 5
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 27
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: fontFilter.activeFocus ? Theme.accent : Theme.border
                TextField {
                    id: fontFilter
                    anchors.fill: parent
                    anchors.leftMargin: 7
                    anchors.rightMargin: 7
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.text
                    placeholderText: qsTr("Search fonts…")
                    placeholderTextColor: Theme.text3
                    background: null
                    font.pixelSize: 11
                    selectByMouse: true
                }
            }
            ListView {
                id: fontList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: fontPopup._filtered()
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    required property int index
                    required property var modelData
                    width: fontList.width
                    height: 26
                    contentItem: Text {
                        text: modelData
                        color: Theme.text
                        font.pixelSize: 12
                        font.family: modelData
                        leftPadding: 7
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle { color: hovered ? Theme.surface2 : "transparent"; radius: 4 }
                    onClicked: {
                        bar._applyStashed(function(d,s,e){ TextFormatter.applyFontFamily(d, s, e, modelData) })
                        fontChip.label = modelData
                        fontPopup.close()
                    }
                }
            }
        }
    }

    // ── Font size popup ───────────────────────────────────────────────────────
    Popup {
        id: sizePopup
        width: 80
        height: 230
        padding: 5
        modal: false
        scale: Theme.uiScale
        transformOrigin: Item.TopLeft
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        // Clicking away dismisses the picker and nothing else — see ClickShield.qml.
        ClickShield { host: sizePopup }

        function openFor(anchorItem) {
            var p = anchorItem.mapToItem(bar, 0, anchorItem.height)
            x = Math.max(0, Math.min(p.x, bar.width - width))
            y = p.y + 4
            open()
        }

        contentItem: ListView {
            clip: true
            model: bar._fontSizes
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                required property var modelData
                width: ListView.view ? ListView.view.width : 70
                height: 26
                contentItem: Text {
                    text: modelData
                    color: Theme.text
                    font.pixelSize: 12
                    leftPadding: 8
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: hovered ? Theme.surface2 : "transparent"; radius: 4 }
                onClicked: {
                    bar._applyStashed(function(d,s,e){ TextFormatter.applyFontPointSize(d, s, e, modelData) })
                    sizePopup.close()
                }
            }
        }
    }

    // ── Paragraph style popup (Normal text + Heading 1–6) ─────────────────────
    Popup {
        id: stylePopup
        width: 160
        padding: 5
        modal: false
        scale: Theme.uiScale
        transformOrigin: Item.TopLeft
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        // Clicking away dismisses the picker and nothing else — see ClickShield.qml.
        ClickShield { host: stylePopup }

        function openFor(anchorItem) {
            var p = anchorItem.mapToItem(bar, 0, anchorItem.height)
            x = Math.max(0, Math.min(p.x, bar.width - width))
            y = p.y + 4
            open()
        }

        contentItem: ColumnLayout {
            spacing: 0
            Repeater {
                model: bar._paragraphStyles
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: 30
                    radius: 4
                    color: stHover.hovered ? Theme.surface2 : "transparent"
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        text: modelData.label
                        color: Theme.text
                        font.pixelSize: modelData.px
                        font.weight: modelData.wt
                    }
                    HoverHandler { id: stHover }
                    TapHandler {
                        onTapped: {
                            bar._applyStashed(function(d,s,e){ TextFormatter.applyStyle(d, s, e, modelData.style) })
                            stylePopup.close()
                        }
                    }
                }
            }
        }
    }

    // ── Font color popup ──────────────────────────────────────────────────────
    Popup {
        id: colorPopup
        width: 210
        padding: 8
        modal: false
        scale: Theme.uiScale
        transformOrigin: Item.TopLeft
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        // Clicking away dismisses the picker and nothing else — see ClickShield.qml.
        ClickShield { host: colorPopup }

        function openFor(anchorItem) {
            var p = anchorItem.mapToItem(bar, 0, anchorItem.height)
            x = Math.max(0, Math.min(p.x, bar.width - width))
            y = p.y + 4
            open()
        }

        contentItem: ColumnLayout {
            spacing: 6
            Text {
                text: qsTr("Text color")
                color: Theme.text3; font.pixelSize: 10; font.weight: Font.DemiBold
            }
            Grid {
                columns: 8
                spacing: 5
                Repeater {
                    model: bar._fontColors
                    delegate: Rectangle {
                        required property string modelData
                        width: 19; height: 19; radius: 9.5
                        color: modelData
                        border.color: (modelData === "#ffffff" || modelData === "#000000") ? Theme.border : "transparent"
                        border.width: 1
                        HoverHandler { id: swHover }
                        Rectangle {
                            anchors.fill: parent
                            radius: 9.5
                            color: "transparent"
                            border.color: Theme.accent
                            border.width: swHover.hovered ? 2 : 0
                        }
                        TapHandler {
                            onTapped: {
                                bar._applyStashed(function(d,s,e){ TextFormatter.applyFontColor(d, s, e, modelData) })
                                colorPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Highlight color popup ─────────────────────────────────────────────────
    Popup {
        id: highlightPopup
        width: 210
        padding: 8
        modal: false
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        // Clicking away dismisses the picker and nothing else — see ClickShield.qml.
        ClickShield { host: highlightPopup }

        function openFor(anchorItem) {
            var p = anchorItem.mapToItem(bar, 0, anchorItem.height)
            x = Math.max(0, Math.min(p.x, bar.width - width))
            y = p.y + 4
            open()
        }

        contentItem: ColumnLayout {
            spacing: 6
            Text {
                text: qsTr("Highlight")
                color: Theme.text3; font.pixelSize: 10; font.weight: Font.DemiBold
            }
            Grid {
                columns: 8
                spacing: 5
                Repeater {
                    model: bar._highlightColors
                    delegate: Rectangle {
                        required property string modelData
                        width: 19; height: 19; radius: 4
                        color: modelData
                        border.color: Theme.border
                        border.width: 1
                        HoverHandler { id: hlHover }
                        Rectangle {
                            anchors.fill: parent
                            radius: 4
                            color: "transparent"
                            border.color: Theme.accent
                            border.width: hlHover.hovered ? 2 : 0
                        }
                        TapHandler {
                            onTapped: {
                                bar._applyStashed(function(d,s,e){ TextFormatter.applyFontHighlight(d, s, e, modelData) })
                                highlightPopup.close()
                            }
                        }
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 24
                radius: Theme.radiusSm
                color: noneHover.hovered ? Theme.surface2 : "transparent"
                border.color: Theme.border
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 7
                    spacing: 5
                    MaterialIcon { name: "format_color_reset"; size: 13; color: Theme.text2 }
                    Text { text: qsTr("No highlight"); color: Theme.text2; font.pixelSize: 11 }
                }
                HoverHandler { id: noneHover }
                TapHandler {
                    onTapped: {
                        bar._applyStashed(function(d,s,e){ TextFormatter.applyFontHighlight(d, s, e, "transparent") })
                        highlightPopup.close()
                    }
                }
            }
        }
    }

    // ── Insert-table grid picker (drag/hover to choose rows × columns) ─────────
    Popup {
        id: tablePopup
        padding: 8
        modal: false
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        // Clicking away dismisses the picker and nothing else — see ClickShield.qml.
        ClickShield { host: tablePopup }

        readonly property int maxRows: 8
        readonly property int maxCols: 10
        readonly property int cell: 16
        readonly property int gap: 2
        // Currently hovered extent; 0 means nothing hovered yet.
        property int hoverRows: 0
        property int hoverCols: 0

        function openFor(anchorItem) {
            hoverRows = 0; hoverCols = 0
            var p = anchorItem.mapToItem(bar, 0, anchorItem.height)
            x = Math.max(0, Math.min(p.x, bar.width - width))
            y = p.y + 4
            open()
        }

        contentItem: ColumnLayout {
            spacing: 6
            Grid {
                id: cellGrid
                columns: tablePopup.maxCols
                spacing: tablePopup.gap
                Repeater {
                    model: tablePopup.maxRows * tablePopup.maxCols
                    delegate: Rectangle {
                        id: cellRect
                        required property int index
                        readonly property int rowN: Math.floor(index / tablePopup.maxCols) + 1
                        readonly property int colN: (index % tablePopup.maxCols) + 1
                        readonly property bool lit: rowN <= tablePopup.hoverRows && colN <= tablePopup.hoverCols
                        width: tablePopup.cell; height: tablePopup.cell; radius: 3
                        color: lit ? Theme.accent : Theme.surface
                        border.color: lit ? Theme.accent : Theme.border
                        border.width: 1
                        HoverHandler {
                            onHoveredChanged: if (hovered) {
                                tablePopup.hoverRows = cellRect.rowN
                                tablePopup.hoverCols = cellRect.colN
                            }
                        }
                        TapHandler {
                            onTapped: {
                                var r = cellRect.rowN, c = cellRect.colN
                                bar._applyStashed(function(d,s,e){
                                    TextFormatter.insertTable(d, e, r, c)
                                })
                                tablePopup.close()
                            }
                        }
                    }
                }
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: tablePopup.hoverRows > 0
                      ? tablePopup.hoverCols + " × " + tablePopup.hoverRows
                      : qsTr("Insert table")
                color: Theme.text2; font.pixelSize: 11
            }
        }
    }
}

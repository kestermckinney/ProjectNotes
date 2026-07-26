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

    implicitHeight: 40
    radius: Theme.radiusSm
    color: Theme.surface2
    border.color: Theme.border

    readonly property var _fontSizes: [8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 32, 36, 48, 72]

    // Paragraph styles for the Style dropdown. `style` maps to TextFormatter.applyStyle:
    // 0 = normal body text, 9..14 = Heading 1..6.
    readonly property var _paragraphStyles: [
        { label: qsTr("Normal text"), style: 0,  px: 13, wt: Font.Normal   },
        { label: qsTr("Heading 1"),   style: 9,  px: 22, wt: Font.Bold     },
        { label: qsTr("Heading 2"),   style: 10, px: 19, wt: Font.Bold     },
        { label: qsTr("Heading 3"),   style: 11, px: 16, wt: Font.DemiBold },
        { label: qsTr("Heading 4"),   style: 12, px: 14, wt: Font.DemiBold },
        { label: qsTr("Heading 5"),   style: 13, px: 13, wt: Font.DemiBold },
        { label: qsTr("Heading 6"),   style: 14, px: 12, wt: Font.DemiBold }
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

    // Immediate ops that use the editor's live selection.
    function _apply(fn) {
        if (!editor) return
        var ss = editor.selectionStart
        var se = editor.selectionEnd
        fn(editor.textDocument, ss, se)
        editor.forceActiveFocus()
        editor.select(ss, se)
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
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        spacing: 2

        component FmtButton: Item {
            id: btn
            property string icon: ""
            property string glyph: ""
            signal triggered()
            implicitWidth: 30
            implicitHeight: 30
            Layout.alignment: Qt.AlignVCenter
            Rectangle {
                anchors.centerIn: parent
                width: 28; height: 28; radius: 6
                color: hh.hovered ? Theme.surface : "transparent"
                MaterialIcon {
                    anchors.centerIn: parent
                    visible: btn.icon !== ""
                    name: btn.icon; size: 18; color: Theme.text2
                }
                Text {
                    anchors.centerIn: parent
                    visible: btn.glyph !== ""
                    text: btn.glyph
                    color: Theme.text2
                    font.pixelSize: 15
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
            property alias contentWidth: chipText.implicitWidth
            signal clicked()
            Layout.alignment: Qt.AlignVCenter
            implicitHeight: 28
            implicitWidth: chipRow.implicitWidth + 14
            radius: 6
            color: chHover.hovered ? Theme.surface : "transparent"
            RowLayout {
                id: chipRow
                anchors.centerIn: parent
                spacing: 3
                Text {
                    id: chipText
                    text: chip.label
                    color: Theme.text2
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    Layout.maximumWidth: 120
                }
                MaterialIcon { name: "arrow_drop_down"; size: 16; color: Theme.text3 }
            }
            HoverHandler { id: chHover }
            TapHandler { onTapped: chip.clicked() }
        }

        component Sep: Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 1; height: 20; color: Theme.border
        }

        FmtButton { glyph: "B"; onTriggered: bar._apply(TextFormatter.toggleBold) }
        FmtButton { icon: "format_italic";        onTriggered: bar._apply(TextFormatter.toggleItalic) }
        FmtButton { icon: "format_underlined";    onTriggered: bar._apply(TextFormatter.toggleUnderline) }
        FmtButton { icon: "strikethrough_s";      onTriggered: bar._apply(TextFormatter.toggleStrikethrough) }
        Sep {}

        // ── Font family ───────────────────────────────────────────────────────
        ChipButton {
            id: fontChip
            label: qsTr("Font")
            onClicked: { bar._capture(); fontPopup.openFor(fontChip) }
        }

        // ── Font size ─────────────────────────────────────────────────────────
        ChipButton {
            id: sizeChip
            label: {
                var s = bar.editor ? TextFormatter.currentFontPointSize(bar.editor.textDocument, bar._selEnd) : 12
                return s + ""
            }
            onClicked: {
                bar._capture()
                sizeChip.label = Qt.binding(function() {
                    return TextFormatter.currentFontPointSize(bar.editor.textDocument, bar._selEnd) + ""
                })
                sizePopup.openFor(sizeChip)
            }
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
                anchors.bottomMargin: 5
                width: 16; height: 3; radius: 1
                color: bar.editor ? TextFormatter.currentFontColor(bar.editor.textDocument, bar._selEnd) : Theme.text
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
        FmtButton { icon: "format_align_left";    onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,0) }) }
        FmtButton { icon: "format_align_center";  onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,1) }) }
        FmtButton { icon: "format_align_right";   onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,2) }) }
        Sep {}

        // ── Paragraph style (headings) ────────────────────────────────────────
        ChipButton {
            id: styleChip
            label: {
                var idx = bar.editor ? TextFormatter.currentParagraphStyle(bar.editor.textDocument, bar._selEnd) : -1
                return bar._styleLabel(idx)
            }
            onClicked: {
                bar._capture()
                styleChip.label = Qt.binding(function() {
                    return bar._styleLabel(TextFormatter.currentParagraphStyle(bar.editor.textDocument, bar._selEnd))
                })
                stylePopup.openFor(styleChip)
            }
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
        width: 240
        height: 320
        padding: 8
        modal: false
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

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
            spacing: 6
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 30
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: fontFilter.activeFocus ? Theme.accent : Theme.border
                TextField {
                    id: fontFilter
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.text
                    placeholderText: qsTr("Search fonts…")
                    placeholderTextColor: Theme.text3
                    background: null
                    font.pixelSize: 12
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
                    height: 30
                    contentItem: Text {
                        text: modelData
                        color: Theme.text
                        font.pixelSize: 13
                        font.family: modelData
                        leftPadding: 8
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
        width: 92
        height: 260
        padding: 6
        modal: false
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

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
                width: ListView.view ? ListView.view.width : 80
                height: 30
                contentItem: Text {
                    text: modelData
                    color: Theme.text
                    font.pixelSize: 13
                    leftPadding: 10
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
        width: 180
        padding: 6
        modal: false
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

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
                    implicitHeight: 34
                    radius: 4
                    color: stHover.hovered ? Theme.surface2 : "transparent"
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
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
        width: 244
        padding: 10
        modal: false
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        function openFor(anchorItem) {
            var p = anchorItem.mapToItem(bar, 0, anchorItem.height)
            x = Math.max(0, Math.min(p.x, bar.width - width))
            y = p.y + 4
            open()
        }

        contentItem: ColumnLayout {
            spacing: 8
            Text {
                text: qsTr("Text color")
                color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold
            }
            Grid {
                columns: 8
                spacing: 6
                Repeater {
                    model: bar._fontColors
                    delegate: Rectangle {
                        required property string modelData
                        width: 22; height: 22; radius: 11
                        color: modelData
                        border.color: (modelData === "#ffffff" || modelData === "#000000") ? Theme.border : "transparent"
                        border.width: 1
                        HoverHandler { id: swHover }
                        Rectangle {
                            anchors.fill: parent
                            radius: 11
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
        width: 244
        padding: 10
        modal: false
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        function openFor(anchorItem) {
            var p = anchorItem.mapToItem(bar, 0, anchorItem.height)
            x = Math.max(0, Math.min(p.x, bar.width - width))
            y = p.y + 4
            open()
        }

        contentItem: ColumnLayout {
            spacing: 8
            Text {
                text: qsTr("Highlight")
                color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold
            }
            Grid {
                columns: 8
                spacing: 6
                Repeater {
                    model: bar._highlightColors
                    delegate: Rectangle {
                        required property string modelData
                        width: 22; height: 22; radius: 5
                        color: modelData
                        border.color: Theme.border
                        border.width: 1
                        HoverHandler { id: hlHover }
                        Rectangle {
                            anchors.fill: parent
                            radius: 5
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
                implicitHeight: 28
                radius: Theme.radiusSm
                color: noneHover.hovered ? Theme.surface2 : "transparent"
                border.color: Theme.border
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    spacing: 6
                    MaterialIcon { name: "format_color_reset"; size: 15; color: Theme.text2 }
                    Text { text: qsTr("No highlight"); color: Theme.text2; font.pixelSize: 12 }
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
        padding: 10
        modal: false
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

        readonly property int maxRows: 8
        readonly property int maxCols: 10
        readonly property int cell: 18
        readonly property int gap: 3
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
            spacing: 8
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
                color: Theme.text2; font.pixelSize: 12
            }
        }
    }
}

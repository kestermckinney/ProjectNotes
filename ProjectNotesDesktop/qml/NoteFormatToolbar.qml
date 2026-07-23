// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import ProjectNotesDesktop

// Formatting toolbar bound to a target TextArea/TextEdit (`editor`). Each button
// applies a TextFormatter operation to the editor's QTextDocument over its
// current selection, then restores focus + selection so the highlight persists.
Rectangle {
    id: bar
    property var editor: null

    implicitHeight: 40
    radius: Theme.radiusSm
    color: Theme.surface2
    border.color: Theme.border

    function _apply(fn) {
        if (!editor) return
        var ss = editor.selectionStart
        var se = editor.selectionEnd
        fn(editor.textDocument, ss, se)
        editor.forceActiveFocus()
        editor.select(ss, se)
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

        component Sep: Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 1; height: 20; color: Theme.border
        }

        FmtButton { glyph: "B"; onTriggered: bar._apply(TextFormatter.toggleBold) }
        FmtButton { icon: "format_italic";        onTriggered: bar._apply(TextFormatter.toggleItalic) }
        FmtButton { icon: "format_underlined";    onTriggered: bar._apply(TextFormatter.toggleUnderline) }
        FmtButton { icon: "strikethrough_s";      onTriggered: bar._apply(TextFormatter.toggleStrikethrough) }
        Sep {}
        FmtButton { icon: "format_list_bulleted"; onTriggered: bar._apply(TextFormatter.toggleBulletList) }
        FmtButton { icon: "format_indent_increase"; onTriggered: bar._apply(TextFormatter.indentText) }
        FmtButton { icon: "format_indent_decrease"; onTriggered: bar._apply(TextFormatter.unindentText) }
        Sep {}
        FmtButton { icon: "format_align_left";    onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,0) }) }
        FmtButton { icon: "format_align_center";  onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,1) }) }
        FmtButton { icon: "format_align_right";   onTriggered: bar._apply(function(d,s,e){ TextFormatter.setAlignment(d,s,e,2) }) }
        Sep {}
        FmtButton { glyph: "H1"; onTriggered: bar._apply(function(d,s,e){ TextFormatter.applyStyle(d,s,e,9) }) }
        FmtButton { glyph: "H2"; onTriggered: bar._apply(function(d,s,e){ TextFormatter.applyStyle(d,s,e,10) }) }
        FmtButton { icon: "format_size";  onTriggered: bar._apply(TextFormatter.increaseFontSize) }
        FmtButton { glyph: "P"; onTriggered: bar._apply(function(d,s,e){ TextFormatter.applyStyle(d,s,e,0) }) }

        Item { Layout.fillWidth: true }
    }
}

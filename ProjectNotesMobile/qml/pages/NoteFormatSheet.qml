// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// NoteFormatSheet — bottom-sheet popup for text formatting, styled after the
// Notes app on iOS/Mac.  Caller sets textDocument/selStart/selEnd before
// calling open().

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Popup {
    id: root

    property var textDocument: null
    property int selStart: 0
    property int selEnd: 0

    property bool _showColors: false
    property color _selectedColor: "#000000"

    // Active-state flags populated in onAboutToShow so the sheet can highlight
    // whichever options are already applied at the cursor / selection.
    property bool _isBold: false
    property bool _isItalic: false
    property bool _isUnderline: false
    property bool _isStrikethrough: false
    property int  _currentAlignment: 0       // 0=left, 1=center, 2=right, 3=justify
    property int  _currentListStyle: -1      // -1=none, 1=disc, 4=decimal, etc.
    property int  _currentParagraphStyle: -1 // -1=none, 0=body, 9=title, 10=heading, 11=subheading

    readonly property var _presetColors: [
        "#000000", "#404040", "#808080", "#c0c0c0",
        "#800000", "#ff0000", "#ff6600", "#ff9900",
        "#006600", "#00aa00", "#003399", "#0055ff",
        "#6600cc", "#cc00cc", "#006666", "#00aaff"
    ]

    readonly property int _baseHeight: 290
    readonly property int _colorSectionH: 100

    // ── Popup geometry — slides up from screen bottom ─────────────────────────
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    x: 0
    y: parent ? parent.height - height : 0
    width: parent ? parent.width : 390
    height: _showColors ? _baseHeight + _colorSectionH : _baseHeight
    padding: 0

    Behavior on height {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 220; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 180; easing.type: Easing.InCubic }
    }

    onAboutToShow: {
        Qt.inputMethod.hide()
        _showColors = false
        if (textDocument) {
            _isBold               = TextFormatter.isBoldAt(textDocument, selStart, selEnd)
            _isItalic             = TextFormatter.isItalicAt(textDocument, selStart, selEnd)
            _isUnderline          = TextFormatter.isUnderlineAt(textDocument, selStart, selEnd)
            _isStrikethrough      = TextFormatter.isStrikethroughAt(textDocument, selStart, selEnd)
            _currentAlignment     = TextFormatter.currentAlignment(textDocument, selStart)
            _currentListStyle     = TextFormatter.currentListStyle(textDocument, selStart)
            _currentParagraphStyle = TextFormatter.currentParagraphStyle(textDocument, selStart)
            _selectedColor        = TextFormatter.currentFontColor(textDocument, selStart)
        }
    }

    background: Rectangle {
        color: palette.base
        radius: 16
        layer.enabled: true
    }

    // Reusable button shape. `active` highlights the button to indicate the
    // formatting it controls is currently applied at the cursor/selection.
    component FormatBtn: ToolButton {
        property bool active: false
        focusPolicy: Qt.NoFocus
        Layout.preferredWidth:  44
        Layout.preferredHeight: 44
        background: Rectangle {
            color: parent.active
                   ? Qt.rgba(palette.highlight.r, palette.highlight.g, palette.highlight.b, 0.20)
                   : "transparent"
            radius: 8
            border.color: parent.active ? palette.highlight : palette.mid
            border.width: 1
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ── Header ────────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            Label {
                anchors.centerIn: parent
                text: qsTr("Format")
                font.pixelSize: 17
                font.weight: Font.DemiBold
            }
            ToolButton {
                anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
                icon.name: "xmark"
                focusPolicy: Qt.NoFocus
                onClicked: root.close()
            }
            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: Theme.mutedText; opacity: 0.25
            }
        }

        // ── Text style row (horizontally scrollable) ──────────────────────────
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            contentHeight: 72
            clip: true

            Row {
                height: 72
                leftPadding:  8
                rightPadding: 8
                spacing: 0

                Repeater {
                    model: [
                        { label: qsTr("Title"),      styleIdx: 9,  ptSize: 22, wt: Font.Bold     },
                        { label: qsTr("Heading"),    styleIdx: 10, ptSize: 17, wt: Font.DemiBold },
                        { label: qsTr("Subheading"), styleIdx: 11, ptSize: 14, wt: Font.Medium   },
                        { label: qsTr("Body"),       styleIdx: 0,  ptSize: 14, wt: Font.Normal   }
                    ]
                    delegate: Item {
                        required property var modelData
                        readonly property bool active: root._currentParagraphStyle === modelData.styleIdx
                        width:  styleLabel.implicitWidth + 20
                        height: 72

                        Rectangle {
                            anchors.centerIn: styleLabel
                            width:  styleLabel.implicitWidth  + 16
                            height: styleLabel.implicitHeight + 8
                            radius: 6
                            visible: parent.active
                            color: Qt.rgba(palette.highlight.r, palette.highlight.g, palette.highlight.b, 0.20)
                            border.color: palette.highlight
                            border.width: 1
                        }
                        Label {
                            id: styleLabel
                            anchors.centerIn: parent
                            text:            modelData.label
                            font.pixelSize:  modelData.ptSize
                            font.weight:     modelData.wt
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                TextFormatter.applyStyle(root.textDocument, root.selStart, root.selEnd, modelData.styleIdx)
                                root.close()
                            }
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.mutedText; opacity: 0.25 }

        // ── Character formatting row ───────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Layout.leftMargin:  16
            Layout.rightMargin: 16
            spacing: 8

            FormatBtn {
                text: "B"; font.bold: true; font.pixelSize: 18
                active: root._isBold
                onClicked: { TextFormatter.toggleBold(root.textDocument, root.selStart, root.selEnd); root.close() }
            }
            FormatBtn {
                text: "I"; font.italic: true; font.pixelSize: 18
                active: root._isItalic
                onClicked: { TextFormatter.toggleItalic(root.textDocument, root.selStart, root.selEnd); root.close() }
            }
            FormatBtn {
                text: "U"; font.underline: true; font.pixelSize: 18
                active: root._isUnderline
                onClicked: { TextFormatter.toggleUnderline(root.textDocument, root.selStart, root.selEnd); root.close() }
            }
            FormatBtn {
                text: "S"; font.strikeout: true; font.pixelSize: 18
                active: root._isStrikethrough
                onClicked: { TextFormatter.toggleStrikethrough(root.textDocument, root.selStart, root.selEnd); root.close() }
            }

            Item { Layout.fillWidth: true }

            // Color toggle — expands inline color swatches
            FormatBtn {
                icon.name: "paintpalette"
                background: Rectangle {
                    radius: 8
                    color: root._showColors ? palette.highlight : "transparent"
                    border.color: root._showColors ? palette.highlight : palette.mid
                    border.width: 1
                }
                onClicked: root._showColors = !root._showColors
            }
        }

        // ── Color swatches (shown when pencil is active) ──────────────────────
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: root._showColors ? root._colorSectionH : 0
            clip: true
            visible: root._showColors

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.mutedText; opacity: 0.25 }

                Flow {
                    Layout.fillWidth: true
                    Layout.leftMargin:  16
                    Layout.rightMargin: 16
                    Layout.topMargin:   10
                    spacing: 8

                    Repeater {
                        model: root._presetColors
                        delegate: Rectangle {
                            id: swatch
                            required property string modelData
                            readonly property bool selected: Qt.colorEqual(root._selectedColor, swatch.modelData)
                            width: 30; height: 30; radius: 15
                            // When selected, fill the outer disc with white so the inner
                            // colored disc (shrunk slightly) sits inside a white ring.
                            color: selected ? "white" : "transparent"

                            Rectangle {
                                anchors.centerIn: parent
                                width:  swatch.selected ? 22 : 30
                                height: swatch.selected ? 22 : 30
                                radius: width / 2
                                color:  swatch.modelData
                                border.color: swatch.modelData === "#ffffff" ? palette.mid : "transparent"
                                border.width: 1
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root._selectedColor = swatch.modelData
                                    TextFormatter.applyFontColor(root.textDocument, root.selStart, root.selEnd, swatch.modelData)
                                    root._showColors = false
                                    root.close()
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.mutedText; opacity: 0.25 }

        // ── List / indent / alignment row ─────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Layout.leftMargin:  16
            Layout.rightMargin: 16
            spacing: 6

            FormatBtn {
                icon.name: "list.bullet"
                active: root._currentListStyle === 1
                onClicked: { TextFormatter.applyStyle(root.textDocument, root.selStart, root.selEnd, 1); root.close() }
            }
            FormatBtn {
                icon.name: "list.number"
                active: root._currentListStyle === 4
                onClicked: { TextFormatter.applyStyle(root.textDocument, root.selStart, root.selEnd, 4); root.close() }
            }
            FormatBtn {
                text: "⇥"; font.pixelSize: 16
                onClicked: { TextFormatter.indentText(root.textDocument, root.selStart, root.selEnd); root.close() }
            }
            FormatBtn {
                text: "⇤"; font.pixelSize: 16
                onClicked: { TextFormatter.unindentText(root.textDocument, root.selStart, root.selEnd); root.close() }
            }

            Item { Layout.fillWidth: true }

            FormatBtn {
                icon.name: "text.alignleft"
                active: root._currentAlignment === 0
                onClicked: { TextFormatter.setAlignment(root.textDocument, root.selStart, root.selEnd, 0); root.close() }
            }
            FormatBtn {
                icon.name: "text.aligncenter"
                active: root._currentAlignment === 1
                onClicked: { TextFormatter.setAlignment(root.textDocument, root.selStart, root.selEnd, 1); root.close() }
            }
            FormatBtn {
                icon.name: "text.alignright"
                active: root._currentAlignment === 2
                onClicked: { TextFormatter.setAlignment(root.textDocument, root.selStart, root.selEnd, 2); root.close() }
            }
        }

        Item { Layout.minimumHeight: 8; Layout.fillHeight: true }
    }
}

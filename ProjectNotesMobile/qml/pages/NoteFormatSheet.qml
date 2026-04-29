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
    property string _selectedColor: "#000000"

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
    }

    background: Rectangle {
        color: palette.base
        radius: 16
        layer.enabled: true
    }

    // Reusable button shape
    component FormatBtn: ToolButton {
        focusPolicy: Qt.NoFocus
        Layout.preferredWidth:  44
        Layout.preferredHeight: 44
        background: Rectangle {
            color: "transparent"
            radius: 8
            border.color: palette.mid
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
                height: 1; color: palette.placeholderText; opacity: 0.25
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
                        width:  styleLabel.implicitWidth + 20
                        height: 72

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

        Rectangle { Layout.fillWidth: true; height: 1; color: palette.placeholderText; opacity: 0.25 }

        // ── Character formatting row ───────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Layout.leftMargin:  16
            Layout.rightMargin: 16
            spacing: 8

            FormatBtn {
                text: "B"; font.bold: true; font.pixelSize: 18
                onClicked: { TextFormatter.toggleBold(root.textDocument, root.selStart, root.selEnd); root.close() }
            }
            FormatBtn {
                text: "I"; font.italic: true; font.pixelSize: 18
                onClicked: { TextFormatter.toggleItalic(root.textDocument, root.selStart, root.selEnd); root.close() }
            }
            FormatBtn {
                text: "U"; font.underline: true; font.pixelSize: 18
                onClicked: { TextFormatter.toggleUnderline(root.textDocument, root.selStart, root.selEnd); root.close() }
            }
            FormatBtn {
                text: "S"; font.strikeout: true; font.pixelSize: 18
                onClicked: { TextFormatter.toggleStrikethrough(root.textDocument, root.selStart, root.selEnd); root.close() }
            }

            Item { Layout.fillWidth: true }

            // Color toggle — expands inline color swatches
            FormatBtn {
                icon.name: "pencil"
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

                Rectangle { Layout.fillWidth: true; height: 1; color: palette.placeholderText; opacity: 0.25 }

                Flow {
                    Layout.fillWidth: true
                    Layout.leftMargin:  16
                    Layout.rightMargin: 16
                    Layout.topMargin:   10
                    spacing: 8

                    Repeater {
                        model: root._presetColors
                        delegate: Rectangle {
                            required property string modelData
                            width: 30; height: 30; radius: 15
                            color: modelData
                            border.color: root._selectedColor === modelData ? palette.highlight
                                         : (modelData === "#ffffff" ? palette.mid : "transparent")
                            border.width: root._selectedColor === modelData ? 3 : 1

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root._selectedColor = modelData
                                    TextFormatter.applyFontColor(root.textDocument, root.selStart, root.selEnd, modelData)
                                    root._showColors = false
                                    root.close()
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: palette.placeholderText; opacity: 0.25 }

        // ── List / indent / alignment row ─────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Layout.leftMargin:  16
            Layout.rightMargin: 16
            spacing: 6

            FormatBtn {
                icon.name: "list.bullet"
                onClicked: { TextFormatter.applyStyle(root.textDocument, root.selStart, root.selEnd, 1); root.close() }
            }
            FormatBtn {
                icon.name: "list.number"
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
                onClicked: { TextFormatter.setAlignment(root.textDocument, root.selStart, root.selEnd, 0); root.close() }
            }
            FormatBtn {
                icon.name: "text.aligncenter"
                onClicked: { TextFormatter.setAlignment(root.textDocument, root.selStart, root.selEnd, 1); root.close() }
            }
            FormatBtn {
                icon.name: "text.alignright"
                onClicked: { TextFormatter.setAlignment(root.textDocument, root.selStart, root.selEnd, 2); root.close() }
            }
        }

        Item { Layout.minimumHeight: 8; Layout.fillHeight: true }
    }
}

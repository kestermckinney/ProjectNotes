// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// A message or confirmation dialog with the same themed chrome as Main.qml's
// error and delete-confirmation dialogs, for components that are not Main.qml
// and so cannot reach those instances.  With no confirmText it shows a single
// OK button; with one it shows Cancel plus that action, styled red when
// destructive.
Dialog {
    id: dialog
    property string message: ""
    property bool selectableMessage: false
    // message is HTML (a subset Qt's RichText understands). Callers must escape
    // any text taken from data; see PluginsSettingsPane.escapeHtml.
    property bool richMessage: false
    property string iconName: "error"
    property color iconColor: Theme.red
    property string confirmText: ""
    property bool destructive: false
    // Wider for long content such as plugin details; never wider than the window.
    property int preferredWidth: 380
    signal confirmed()

    // The window size in the dialog's own (unscaled) units, less a margin, so
    // the dialog always fits.  The message scrolls; the buttons stay visible.
    readonly property real fitWidth: (parent ? parent.width / Theme.uiScale : 800) - 48
    readonly property real fitHeight: (parent ? parent.height / Theme.uiScale : 600) - 48
    readonly property real maxMessageHeight:
        Math.max(80, fitHeight - headerRow.implicitHeight - 28 - 1
                     - buttonRow.implicitHeight - 18 * 2 - 16)

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.max(280, Math.min(preferredWidth, fitWidth))
    scale: Theme.uiScale   // match the zoomed workspace (centered origin)
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape

    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    header: null
    footer: null

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            id: headerRow
            Layout.fillWidth: true
            Layout.margins: 14
            spacing: 8
            MaterialIcon { name: dialog.iconName; size: 20; color: dialog.iconColor }
            Text {
                text: dialog.title
                color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
                Layout.fillWidth: true
            }
            MaterialIcon {
                name: "close"; size: 20; color: Theme.text3
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dialog.close() }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 18
            spacing: 16
            ScrollView {
                id: messageScroll
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, dialog.maxMessageHeight)
                contentWidth: availableWidth
                contentHeight: dialog.selectableMessage ? messageText.implicitHeight
                                                        : messageLabel.implicitHeight
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                Text {
                    id: messageLabel
                    visible: !dialog.selectableMessage
                    width: messageScroll.availableWidth
                    rightPadding: 10   // room for the scroll bar
                    wrapMode: Text.Wrap
                    color: Theme.text
                    font.pixelSize: Theme.fontLg
                    textFormat: dialog.richMessage ? Text.RichText : Text.PlainText
                    text: dialog.message
                }
                TextArea {
                    id: messageText
                    visible: dialog.selectableMessage
                    width: messageScroll.availableWidth
                    rightPadding: 10
                    text: dialog.message
                    textFormat: TextEdit.PlainText
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    color: Theme.text
                    font.pixelSize: Theme.fontLg
                    background: null
                    padding: 0
                    SpellCheckField { target: messageText; spellCheckEnabled: false }
                }
            }
            RowLayout {
                id: buttonRow
                Layout.fillWidth: true
                spacing: 10
                Item { Layout.fillWidth: true }
                Rectangle {
                    visible: dialog.confirmText !== ""
                    implicitHeight: 30
                    implicitWidth: cancelText.implicitWidth + 26
                    radius: Theme.radiusSm
                    color: cancelHover.hovered ? Theme.surface2 : Theme.surface
                    border.color: Theme.border
                    Text {
                        id: cancelText
                        anchors.centerIn: parent
                        text: qsTr("Cancel")
                        color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                    }
                    HoverHandler { id: cancelHover }
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dialog.close() }
                }
                Rectangle {
                    implicitHeight: 30
                    implicitWidth: actionText.implicitWidth + 26
                    radius: Theme.radiusSm
                    color: dialog.destructive
                           ? (actionHover.hovered ? Theme.red : Theme.redSoft)
                           : (actionHover.hovered ? Theme.accentStrong : Theme.accent)
                    border.color: dialog.destructive ? Theme.red : "transparent"
                    Text {
                        id: actionText
                        anchors.centerIn: parent
                        text: dialog.confirmText !== "" ? dialog.confirmText : qsTr("OK")
                        color: dialog.destructive && !actionHover.hovered ? Theme.red : "#ffffff"
                        font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                    }
                    HoverHandler { id: actionHover }
                    // Exclusive grab keeps the tap from also reaching whatever
                    // sits under the dialog.
                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                        onTapped: {
                            dialog.close()
                            if (dialog.confirmText !== "")
                                dialog.confirmed()
                        }
                    }
                }
            }
        }
    }
}

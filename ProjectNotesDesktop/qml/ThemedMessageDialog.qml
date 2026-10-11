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
    property string iconName: "error"
    property color iconColor: Theme.red
    property string confirmText: ""
    property bool destructive: false
    signal confirmed()

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 380
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
            Text {
                visible: !dialog.selectableMessage
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: Theme.text
                font.pixelSize: Theme.fontLg
                text: dialog.message
            }
            ScrollView {
                id: messageScroll
                visible: dialog.selectableMessage
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(messageText.implicitHeight, 360)
                contentWidth: availableWidth
                clip: true
                TextArea {
                    id: messageText
                    width: messageScroll.availableWidth
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

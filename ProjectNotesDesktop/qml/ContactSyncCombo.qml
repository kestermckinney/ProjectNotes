// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls.Basic
import ProjectNotesDesktop

KeyboardComboBox {
    id: control
    implicitHeight: 32
    background: Rectangle {
        color: Theme.surface
        radius: Theme.radiusSm
        border.color: Theme.border
    }
    contentItem: Text {
        text: control.displayText
        color: Theme.text
        leftPadding: 9
        rightPadding: 26
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: MaterialIcon {
        name: "expand_more"
        size: 16
        color: Theme.text3
        anchors.right: parent.right
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
    }
    delegate: ItemDelegate {
        required property int index
        width: control.width
        contentItem: Text {
            text: control.textAt(index)
            color: Theme.text
            font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
        }
        background: Rectangle {
            color: highlighted ? Theme.accentSoft : Theme.raise
        }
        highlighted: control.highlightedIndex === index
    }
    popup: Popup {
        y: control.height + 2
        width: control.width
        padding: 1
        implicitHeight: Math.min(contentItem.implicitHeight + 2, 280)
        background: Rectangle {
            color: Theme.raise
            radius: Theme.radiusSm
            border.color: Theme.border
        }
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }
}

// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls.Basic
import ProjectNotesDesktop

KeyboardComboBox {
    id: control
    implicitHeight: 32
    font.pixelSize: Theme.fontBody
    contentItem: Text {
        text: control.displayText
        color: Theme.text
        font: control.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        anchors.fill: parent
        anchors.leftMargin: 9
        anchors.rightMargin: 28
    }
    background: Rectangle {
        radius: Theme.radiusSm
        color: Theme.surface
        border.color: control.activeFocus ? Theme.accent : Theme.border
    }
    indicator: MaterialIcon {
        name: "expand_more"
        size: 16
        color: Theme.text3
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 7
    }
    popup: Popup {
        y: control.height + 2
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 2, 280)
        padding: 1
        background: Rectangle {
            radius: Theme.radiusSm
            color: Theme.raise
            border.color: Theme.border
        }
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }
    delegate: ItemDelegate {
        id: itemDelegate
        // `index` is a delegate context property, not an ItemDelegate
        // property.  Declare it so every popup row receives its own model
        // index; accessing `itemDelegate.index` instead resolved to an
        // undefined value and made every row display the first option.
        required property int index
        width: control.width
        contentItem: Text {
            text: control.textAt(itemDelegate.index)
            color: Theme.text
            font.pixelSize: Theme.fontBody
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: itemDelegate.highlighted ? Theme.accentSoft : "transparent"
        }
        highlighted: control.highlightedIndex === itemDelegate.index
        onClicked: {
            control.currentIndex = itemDelegate.index
            control.activated(itemDelegate.index)
            control.popup.close()
        }
    }
}

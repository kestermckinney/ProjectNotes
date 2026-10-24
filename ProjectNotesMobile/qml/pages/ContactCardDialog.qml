// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// ContactCardDialog — centered modal dialog triggered by long-pressing a
// contact, team member, or meeting attendee row. Offers the contact card
// (vCard) actions from the desktop's right-click menu: Email Contact… and
// Export Contact…. On the Contacts list, which used to open Quick Filter
// on long press, a "Quick Filter…" row hands off to that dialog.
// Usage:
//   ContactCardDialog { id: cardDialog; onQuickFilterRequested: qfDialog.openWith(...) }
//   cardDialog.openFor(personId, name, /*showQuickFilter*/ false)

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Popup {
    id: root

    property string _contactId: ""
    property string _name: ""
    property bool _showQuickFilter: false

    signal quickFilterRequested()

    // ── Public API ────────────────────────────────────────────────────────────
    function openFor(contactId, name, showQuickFilter) {
        _contactId = contactId || ""
        _name = name || ""
        _showQuickFilter = showQuickFilter === true
        if (_contactId === "" && !_showQuickFilter) return
        open()
    }

    // Same geometry and dismissal as QuickFilterDialog.qml; see there for why
    // outside taps close through the backdrop TapHandler rather than
    // CloseOnPressOutside (the long-press release would close it at once).
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    width: Math.min(340, (parent ? parent.width : 390) - 48)
    padding: 0
    topPadding: 0
    closePolicy: Popup.CloseOnEscape
    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.45)
        TapHandler { onTapped: root.close() }
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale";   from: 0.92; to: 1; duration: 160; easing.type: Easing.OutCubic }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 130; easing.type: Easing.InCubic }
            NumberAnimation { property: "scale";   from: 1; to: 0.96; duration: 130; easing.type: Easing.InCubic }
        }
    }

    background: Rectangle {
        color: palette.base
        radius: 16
        layer.enabled: true
    }

    contentItem: ColumnLayout {
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Label {
                anchors { left: parent.left; right: closeBtn.left; leftMargin: 44; verticalCenter: parent.verticalCenter }
                horizontalAlignment: Text.AlignHCenter
                text: root._name !== "" ? root._name : qsTr("Contact")
                font.pixelSize: Theme.headline; font.letterSpacing: Theme.tracking(Theme.headline)
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            ToolButton {
                id: closeBtn
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                icon.name: "xmark"
                focusPolicy: Qt.NoFocus
                onClicked: root.close()
            }
            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: Theme.mutedText; opacity: 0.25
            }
        }

        ItemDelegate {
            visible: root._contactId !== ""
            Layout.fillWidth: true
            icon.name: "envelope"
            text: qsTr("Email Contact…")
            onClicked: {
                var id = root._contactId
                root.close()
                AppController.emailContactCard(id)
            }
        }

        ItemDelegate {
            visible: root._contactId !== ""
            Layout.fillWidth: true
            icon.name: "square.and.arrow.up"
            text: qsTr("Export Contact…")
            onClicked: {
                var id = root._contactId
                root.close()
                AppController.shareContactCard(id)
            }
        }

        ItemDelegate {
            visible: root._showQuickFilter
            Layout.fillWidth: true
            icon.name: "line.3.horizontal.decrease.circle"
            text: qsTr("Quick Filter…")
            onClicked: {
                root.close()
                root.quickFilterRequested()
            }
        }

        Item { Layout.preferredHeight: 8 }
    }
}

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Clients master list (cards). Click a row to open ClientDetailPage.
Item {
    id: page
    signal clientActivated(int row, string clientId)
    signal exportRequested(string table, string id)
    signal filterRequested()
    signal sortRequested(real sx, real sy)

    property int    _ctxRow: -1
    property string _ctxId: ""

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Client")
        model: DesktopAppController.clientsModel
        recordId: page._ctxId
        onOpenRequested:   page.clientActivated(page._ctxRow, page._ctxId)
        onNewRequested: {
            var cr = DesktopAppController.addClient()
            if (cr >= 0) page.clientActivated(cr, DesktopAppController.clientIdAtProxyRow(cr))
        }
        onDeleteRequested: DesktopAppController.deleteClient(page._ctxRow)
        onDuplicateRequested: {
            var newId = DesktopAppController.duplicateRecord(DesktopAppController.clientsModel, page._ctxId)
            if (newId !== "") page.clientActivated(DesktopAppController.clientRowForId(newId), newId)
        }
        onExportRequested: page.exportRequested("clients", page._ctxId)
        onFilterRequested: page.filterRequested()
        onSortRequested: (sx, sy) => page.sortRequested(sx, sy)
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.clientsModel)
    }

    // Virtualized list — only visible cards (plus cacheBuffer) are instantiated,
    // and reuseItems recycles delegates while scrolling.
    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        spacing: 7
        model: DesktopAppController.clientsModel
        reuseItems: true
        cacheBuffer: 800
        boundsBehavior: Flickable.StopAtBounds
        footer: Item { width: 1; height: 6 }
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string cid: model.id !== undefined ? model.id : ""
                    width: ListView.view ? ListView.view.width : 0
                    implicitHeight: 48
                    color: hover.hovered ? Theme.raise : Theme.surface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 11
                        Rectangle {
                            width: 26; height: 26; radius: Theme.radiusSm
                            color: Theme.accentSoft
                            MaterialIcon { anchors.centerIn: parent; name: "apartment"; size: 15; color: Theme.accent }
                        }
                        Text {
                            text: (card.model.client_name || qsTr("(no name)")).toString()
                            color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold
                            elide: Text.ElideRight; Layout.fillWidth: true
                        }
                        KebabButton {
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: (sx, sy) => card._openMenu(sx, sy)
                        }
                        MaterialIcon { name: "chevron_right"; size: 17; color: Theme.text3 }
                    }

                    // Populate the shared context menu for this row and open it at
                    // the given scene coordinates — shared by right-click and kebab.
                    function _openMenu(sx, sy) {
                        page._ctxRow = card.index
                        page._ctxId = card.cid
                        ctxMenu.recordLabel = (card.model.client_name || "").toString()
                        ctxMenu.openAt(sx, sy)
                    }

                    HoverHandler { id: hover }
                    TapHandler { onTapped: page.clientActivated(card.index, card.cid) }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => card._openMenu(ev.scenePosition.x, ev.scenePosition.y)
                    }
        }
    }
}

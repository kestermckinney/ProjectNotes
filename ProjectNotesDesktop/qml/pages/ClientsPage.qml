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

    property int    _ctxRow: -1
    property string _ctxId: ""

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Client")
        onOpenRequested:   page.clientActivated(page._ctxRow, page._ctxId)
        onNewRequested: {
            var cr = DesktopAppController.addClient()
            if (cr >= 0) page.clientActivated(cr, DesktopAppController.clientIdAtProxyRow(cr))
        }
        onDeleteRequested: DesktopAppController.deleteClient(page._ctxRow)
        onExportRequested: page.exportRequested("clients", page._ctxId)
        onFilterRequested: page.filterRequested()
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.clientsModel)
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: page.width - 32
            spacing: 10

            Repeater {
                model: DesktopAppController.clientsModel
                delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string cid: model.id !== undefined ? model.id : ""
                    Layout.fillWidth: true
                    implicitHeight: 56
                    color: hover.hovered ? Theme.raise : Theme.surface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 14
                        Rectangle {
                            width: 30; height: 30; radius: 8
                            color: Theme.accentSoft
                            MaterialIcon { anchors.centerIn: parent; name: "apartment"; size: 17; color: Theme.accent }
                        }
                        Text {
                            text: (card.model.client_name || qsTr("(no name)")).toString()
                            color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold
                            elide: Text.ElideRight; Layout.fillWidth: true
                        }
                        MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3 }
                    }
                    HoverHandler { id: hover }
                    TapHandler { onTapped: page.clientActivated(card.index, card.cid) }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => {
                            page._ctxRow = card.index
                            page._ctxId = card.cid
                            ctxMenu.recordLabel = (card.model.client_name || "").toString()
                            ctxMenu.openAt(ev.scenePosition.x, ev.scenePosition.y)
                        }
                    }
                }
            }
            Item { Layout.preferredHeight: 8 }
        }
    }
}

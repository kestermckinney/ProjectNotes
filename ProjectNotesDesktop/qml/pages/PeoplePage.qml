// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// People master list (cards). Click a row to open PersonDetailPage.
Item {
    id: page
    signal personActivated(int row, string personId)
    signal exportRequested(string table, string id)
    signal filterRequested()
    signal sortRequested(real sx, real sy)
    signal goToClientRequested(string clientId)

    property int    _ctxRow: -1
    property string _ctxId: ""

    // Quick Filter entry for a right-clicked person row: pre-filled from that
    // row's own client — client_id is a genuine direct column on the people
    // table (peoplemodel.cpp), not derived, so no lookup fix was needed here.
    function _quickFiltersForRow(m) {
        var qf = []
        var clientId = (m.client_id || "").toString()
        if (clientId !== "")
            qf.push({ icon: "apartment", label: qsTr("This Client"), field: "client_id", values: [clientId] })
        return qf
    }

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Person")
        model: DesktopAppController.peopleModel
        recordId: page._ctxId
        onOpenRequested:   page.personActivated(page._ctxRow, page._ctxId)
        onNewRequested: {
            var pr = DesktopAppController.addPerson()
            if (pr >= 0) page.personActivated(pr, DesktopAppController.personIdAtRow(pr))
        }
        onDeleteRequested: DesktopAppController.deletePerson(page._ctxRow)
        onExportRequested: page.exportRequested("people", page._ctxId)
        onFilterRequested: page.filterRequested()
        onSortRequested: (sx, sy) => page.sortRequested(sx, sy)
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.peopleModel)
        onGoToClientRequested: page.goToClientRequested(clientId)
    }

    // Virtualized list — only visible cards (plus cacheBuffer) are instantiated,
    // and reuseItems recycles delegates while scrolling.
    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        spacing: 10
        model: DesktopAppController.peopleModel
        reuseItems: true
        cacheBuffer: 800
        boundsBehavior: Flickable.StopAtBounds
        footer: Item { width: 1; height: 8 }
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string pid: model.id !== undefined ? model.id : ""
                    width: ListView.view ? ListView.view.width : 0
                    implicitHeight: 64
                    color: hover.hovered ? Theme.raise : Theme.surface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 14
                        Rectangle {
                            width: 36; height: 36; radius: 18
                            color: Theme.accentSoft
                            Text {
                                anchors.centerIn: parent
                                text: {
                                    var n = (card.model.name || "").toString().trim()
                                    if (n === "") return "?"
                                    var p = n.split(" ")
                                    return (p[0][0] || "") + (p.length > 1 ? p[p.length-1][0] : "")
                                }
                                color: Theme.accent; font.pixelSize: 13; font.weight: Font.Bold
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: (card.model.name || qsTr("(no name)")).toString()
                                color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                            Text {
                                text: {
                                    var e = (card.model.email || "").toString()
                                    var r = (card.model.role || "").toString()
                                    return [r, e].filter(function(x){ return x !== "" }).join("  ·  ")
                                }
                                color: Theme.text3; font.pixelSize: 12
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                        }
                        KebabButton {
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: (sx, sy) => card._openMenu(sx, sy)
                        }
                        MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3 }
                    }

                    // Populate the shared context menu for this row and open it at
                    // the given scene coordinates — shared by right-click and the
                    // kebab button.
                    function _openMenu(sx, sy) {
                        page._ctxRow = card.index
                        page._ctxId = card.pid
                        ctxMenu.recordLabel = (card.model.name || "").toString()
                        ctxMenu.clientId = (card.model.client_id || "").toString()
                        ctxMenu.quickFilters = page._quickFiltersForRow(card.model)
                        ctxMenu.openAt(sx, sy)
                    }

                    HoverHandler { id: hover }
                    TapHandler { onTapped: page.personActivated(card.index, card.pid) }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => card._openMenu(ev.scenePosition.x, ev.scenePosition.y)
                    }
        }
    }
}

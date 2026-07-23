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

    property int    _ctxRow: -1
    property string _ctxId: ""

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Person")
        onOpenRequested:   page.personActivated(page._ctxRow, page._ctxId)
        onNewRequested: {
            var pr = DesktopAppController.addPerson()
            if (pr >= 0) page.personActivated(pr, DesktopAppController.personIdAtRow(pr))
        }
        onDeleteRequested: DesktopAppController.deletePerson(page._ctxRow)
        onExportRequested: page.exportRequested("people", page._ctxId)
        onFilterRequested: page.filterRequested()
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.peopleModel)
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
                model: DesktopAppController.peopleModel
                delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string pid: model.id !== undefined ? model.id : ""
                    Layout.fillWidth: true
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
                        MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3 }
                    }
                    HoverHandler { id: hover }
                    TapHandler { onTapped: page.personActivated(card.index, card.pid) }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => {
                            page._ctxRow = card.index
                            page._ctxId = card.pid
                            ctxMenu.recordLabel = (card.model.name || "").toString()
                            ctxMenu.openAt(ev.scenePosition.x, ev.scenePosition.y)
                        }
                    }
                }
            }
            Item { Layout.preferredHeight: 8 }
        }
    }
}

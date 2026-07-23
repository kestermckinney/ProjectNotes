// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// All tracker items (risks / issues / action items) across projects.
Item {
    id: page
    signal itemActivated(string itemId)
    signal exportRequested(string table, string id)
    signal filterRequested()

    property string _ctxId: ""

    Component.onCompleted: DesktopAppController.refreshAllItems()

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Item")
        canDelete: false            // master list is a read-only view
        onOpenRequested:   page.itemActivated(page._ctxId)
        onNewRequested:    page.itemActivated(page._ctxId)
        onExportRequested: page.exportRequested("item_tracker", page._ctxId)
        onFilterRequested: page.filterRequested()
        onRefreshRequested: DesktopAppController.refreshAllItems()
    }

    function _statusColor(s) {
        s = (s || "").toLowerCase()
        if (s.indexOf("resolved") >= 0 || s.indexOf("closed") >= 0) return Theme.green
        if (s.indexOf("assigned") >= 0) return Theme.amber
        if (s.indexOf("new") >= 0) return Theme.red
        return Theme.text3
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 16
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: page.width - 32
            spacing: 8

            Repeater {
                model: DesktopAppController.allItemsModel
                delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string iid: model.id !== undefined ? model.id : ""
                    Layout.fillWidth: true
                    implicitHeight: 62
                    color: hover.hovered ? Theme.raise : Theme.surface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        // type badge
                        Rectangle {
                            width: 30; height: 30; radius: 8
                            color: Theme.accentSoft
                            MaterialIcon {
                                anchors.centerIn: parent
                                name: {
                                    var t = (card.model.item_type || "").toString().toLowerCase()
                                    if (t.indexOf("risk") >= 0) return "warning"
                                    if (t.indexOf("issue") >= 0) return "error"
                                    return "task_alt"
                                }
                                size: 16; color: Theme.accent
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            RowLayout {
                                spacing: 8
                                Text {
                                    text: (card.model.item_number || "").toString()
                                    color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold
                                }
                                Text {
                                    text: (card.model.item_name || qsTr("(unnamed)")).toString()
                                    color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold
                                    elide: Text.ElideRight; Layout.fillWidth: true
                                }
                            }
                            Text {
                                text: {
                                    var proj = (card.model.project_number || "").toString()
                                    var pn = (card.model.project_name || "").toString()
                                    var pr = (card.model.priority || "").toString()
                                    return [proj + " " + pn, pr].filter(function(x){ return x.trim() !== "" }).join("  ·  ")
                                }
                                color: Theme.text3; font.pixelSize: 11
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                        }
                        // status pill
                        Rectangle {
                            readonly property color c: page._statusColor((card.model.status || "").toString())
                            visible: (card.model.status || "").toString() !== ""
                            radius: 5
                            color: Qt.rgba(c.r, c.g, c.b, 0.14)
                            implicitHeight: 18; implicitWidth: stt.implicitWidth + 14
                            Text {
                                id: stt; anchors.centerIn: parent
                                text: (card.model.status || "").toString()
                                color: parent.c; font.pixelSize: 10; font.weight: Font.DemiBold
                            }
                        }
                        MaterialIcon { name: "chevron_right"; size: 20; color: Theme.text3 }
                    }
                    HoverHandler { id: hover }
                    TapHandler { onTapped: page.itemActivated(card.iid) }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (ev) => {
                            page._ctxId = card.iid
                            ctxMenu.recordLabel = (card.model.item_name || "").toString()
                            ctxMenu.openAt(ev.scenePosition.x, ev.scenePosition.y)
                        }
                    }
                }
            }
            Item { Layout.preferredHeight: 8 }
        }
    }
}

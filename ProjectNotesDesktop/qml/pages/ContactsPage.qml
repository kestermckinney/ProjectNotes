// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Contacts master list (cards). Click a row to open ContactDetailPage.
Item {
    id: page
    signal contactActivated(int row, string contactId)
    signal exportRequested(string table, string id)
    signal filterRequested()
    signal sortRequested(real sx, real sy)
    signal goToCompanyRequested(string companyId)

    property int    _ctxRow: -1
    property string _ctxId: ""

    // Quick Filter entry for a right-clicked contact row: pre-filled from that
    // row's own company — client_id is a genuine direct column on the people
    // table (contactsmodel.cpp), not derived, so no lookup fix was needed here.
    function _quickFiltersForRow(m) {
        var qf = []
        var companyId = (m.client_id || "").toString()
        if (companyId !== "")
            qf.push({ icon: "apartment", label: qsTr("This Company"), field: "client_id", values: [companyId] })
        return qf
    }

    RecordContextMenu {
        id: ctxMenu
        recordType: qsTr("Contact")
        model: DesktopAppController.contactsModel
        recordId: page._ctxId
        onOpenRequested:   page.contactActivated(page._ctxRow, page._ctxId)
        onNewRequested: {
            var pr = DesktopAppController.addContact()
            if (pr >= 0) page.contactActivated(pr, DesktopAppController.contactIdAtRow(pr))
        }
        onDeleteRequested: DesktopAppController.deleteContact(page._ctxRow)
        onDuplicateRequested: {
            var newId = DesktopAppController.duplicateRecord(DesktopAppController.contactsModel, page._ctxId)
            if (newId !== "") page.contactActivated(DesktopAppController.contactsRowForId(newId), newId)
        }
        onExportRequested: page.exportRequested("people", page._ctxId)
        onFilterRequested: page.filterRequested()
        onSortRequested: (sx, sy) => page.sortRequested(sx, sy)
        onRefreshRequested: DesktopAppController.refreshModel(DesktopAppController.contactsModel)
        onGoToCompanyRequested: page.goToCompanyRequested(companyId)
    }

    // Drop a vCard (a .vcf file, or a contact dragged straight from Outlook /
    // Contacts) anywhere on the list to add it: the contact's company is
    // associated with a matching company (or a new one is created), and the
    // contact is added. See DesktopAppController::addContactsFromVCardDrop().
    DropArea {
        anchors.fill: parent
        // Ignore this app's own contact cards being dragged out (ContactDragArea).
        onEntered: (drag) => { if (drag.source && drag.source.isContactDrag) drag.accepted = false }
        onDropped: (drop) => {
            if (drop.source && drop.source.isContactDrag) { drop.accepted = false; return }
            var urls = []
            if (drop.hasUrls)
                for (var i = 0; i < drop.urls.length; i++)
                    urls.push(drop.urls[i].toString())
            var text = ""
            for (var i = 0; i < drop.formats.length; i++) {
                if (drop.formats[i].toLowerCase().indexOf("vcard") !== -1) {
                    text = drop.getDataAsString(drop.formats[i])
                    break
                }
            }
            if (text === "" && drop.hasText && drop.text.indexOf("BEGIN:VCARD") !== -1)
                text = drop.text
            if (urls.length === 0 && text === "") { drop.accepted = false; return }
            DesktopAppController.addContactsFromVCardDrop(urls, text)
            drop.accept()
        }
    }

    // Virtualized list — only visible cards (plus cacheBuffer) are instantiated,
    // and reuseItems recycles delegates while scrolling.
    KeyboardListView {
        id: list
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        spacing: 7
        model: DesktopAppController.contactsModel
        reuseItems: true
        cacheBuffer: 800
        boundsBehavior: Flickable.StopAtBounds
        footer: Item { width: 1; height: 6 }
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: Card {
                    id: card
                    required property int index
                    required property var model
                    readonly property string pid: model.id !== undefined ? model.id : ""
                    width: ListView.view ? ListView.view.width : 0
                    implicitHeight: 54
                    color: hover.hovered ? Theme.raise : Theme.surface

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 11
                        Rectangle {
                            width: 30; height: 30; radius: 15
                            color: Theme.accentSoft
                            Text {
                                anchors.centerIn: parent
                                text: {
                                    var n = (card.model.name || "").toString().trim()
                                    if (n === "") return "?"
                                    var p = n.split(" ")
                                    return (p[0][0] || "") + (p.length > 1 ? p[p.length-1][0] : "")
                                }
                                color: Theme.accent; font.pixelSize: Theme.fontBody; font.weight: Font.Bold
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: (card.model.name || qsTr("(no name)")).toString()
                                color: Theme.text; font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                            Text {
                                text: {
                                    var e = (card.model.email || "").toString()
                                    var r = (card.model.role || "").toString()
                                    return [r, e].filter(function(x){ return x !== "" }).join("  ·  ")
                                }
                                color: Theme.text3; font.pixelSize: Theme.fontSm
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                        }
                        KebabButton {
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: (sx, sy) => card._openMenu(sx, sy)
                        }
                        MaterialIcon { name: "chevron_right"; size: 17; color: Theme.text3 }
                    }

                    // Populate the shared context menu for this row and open it at
                    // the given scene coordinates — shared by right-click and the
                    // kebab button.
                    function _openMenu(sx, sy) {
                        page._ctxRow = card.index
                        page._ctxId = card.pid
                        ctxMenu.recordLabel = (card.model.name || "").toString()
                        ctxMenu.companyId = (card.model.client_id || "").toString()
                        ctxMenu.quickFilters = page._quickFiltersForRow(card.model)
                        ctxMenu.openAt(sx, sy)
                    }

                    HoverHandler { id: hover }
                    function openRecord() { page.contactActivated(card.index, card.pid) }
                    // Click to open, right-click for the menu, drag out to
                    // export the contact as a vCard.
                    ContactDragArea {
                        objectName: "contactDragArea"
                        anchors.fill: parent
                        z: -1
                        contactId: card.pid
                        onActivated: card.openRecord()
                        onMenuRequested: (sx, sy) => card._openMenu(sx, sy)
                    }
        }
    }
}

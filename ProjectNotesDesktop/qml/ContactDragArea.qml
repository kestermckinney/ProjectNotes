// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import ProjectNotesDesktop

// Mouse handling for a contact row that can be dragged out of Project Notes as
// a contact card (vCard). Dropped on Finder / Explorer / a file manager it
// becomes "<name>.vcf"; dropped on a mail or contacts app it arrives as a
// vCard. See DesktopAppController::contactVCardDragData().
//
// A MouseArea rather than a DragHandler for the reason FolderGroup.qml gives:
// preventStealing keeps the enclosing list from turning the drag into a
// scroll. Because it takes the press, it also reports the row's left click
// (activated) and right click (menuRequested, in scene coordinates); give it
// z: -1 under the row's content so buttons and fields keep their own presses.
//
// The drag is a native one (Drag.Automatic), so it runs until the drop and
// leaves Project Notes. vCard DropAreas in this app ignore it (see
// isContactDrag) so a card dropped back on its own list isn't imported again.
MouseArea {
    id: area

    property string contactId: ""
    // The item drawn under the cursor while dragging; the row itself by default.
    property Item dragImageItem: parent

    signal activated()
    signal menuRequested(real sx, real sy)

    // Marks a drag as one of this app's own contact cards: DropArea handlers
    // check drop.source.isContactDrag to skip it.
    readonly property bool isContactDrag: true

    preventStealing: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    drag.target: area.contactId !== "" ? dragProxy : null
    drag.threshold: 6

    onPressed: (mouse) => {
        // Render the drag image ahead of time; grabToImage is asynchronous
        // and the drag starts as soon as the threshold is crossed.
        var item = area.dragImageItem
        if (mouse.button !== Qt.LeftButton || area.contactId === "" || !item || item.width <= 0)
            return
        var scale = Math.min(1, 320 / item.width)
        item.grabToImage(function(result) { dragProxy.Drag.imageSource = result.url },
                         Qt.size(item.width * scale, item.height * scale))
    }
    onClicked: (mouse) => {
        if (mouse.button === Qt.RightButton) {
            var p = area.mapToItem(null, mouse.x, mouse.y)
            area.menuRequested(p.x, p.y)
        } else {
            area.activated()
        }
    }

    drag.onActiveChanged: {
        if (!drag.active) return
        var data = DesktopAppController.contactVCardDragData(area.contactId)
        if (Object.keys(data).length === 0) return
        dragProxy.Drag.mimeData = data
        // Runs the platform drag until the drop, then returns.
        dragProxy.Drag.active = true
        dragProxy.x = 0
        dragProxy.y = 0
    }

    Item {
        id: dragProxy
        Drag.dragType: Drag.Automatic
        Drag.source: area
        Drag.supportedActions: Qt.CopyAction
        Drag.proposedAction: Qt.CopyAction
        Drag.hotSpot.x: 16
        Drag.hotSpot.y: 16
    }
}

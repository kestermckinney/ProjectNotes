// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
// `host` is typed against the *template* Popup, not the Basic style's: styled
// Popup and styled Dialog are unrelated QML types, so a Dialog can't be
// assigned to a `property Popup` — only to their shared T.Popup base.
import QtQuick.Templates as T

// An invisible, full-window modal popup that stays open *underneath* a menu or
// dialog for exactly as long as that popup is open, so the click that dismisses
// it only dismisses it — instead of also activating whatever sits behind.
//
// Why it's needed: Qt dismisses a popup on the mouse *press* that lands outside
// it (Popup.CloseOnPressOutside), and the moment that press closes the last
// modal popup the window overlay stops blocking — mid-delivery. Pointer handlers
// further back in the scene are still offered the very same press, so clicking
// away from a menu also "tapped" the record card, tab or button under the cursor
// and navigated there. Items that handle mouse events themselves (MouseArea,
// Button, ItemDelegate) are never reached, which is why the card lists — whose
// rows are driven by TapHandler — were the visible victims.
//
// Keeping one more modal popup open across the dismissal leaves the overlay
// blocking for the whole delivery, so nothing behind the menu reacts. This one
// is invisible, empty, never closes on its own (NoAutoClose), and sits half a
// step below its host in the overlay — so the menu it shields, and any submenu
// opened over that menu, keep receiving input exactly as before. A host raised
// above z 0 (ComboField's drop-down) puts its shield above a dialog it sits in,
// so a click away from the drop-down doesn't also land on that dialog.
//
// Usage — declare one inside the popup it protects:
//     Popup { id: menu;  ClickShield { host: menu }  /* … */ }
//
// Worth having on any popup whose closePolicy includes CloseOnPressOutside
// (that's the Popup default); popups that only close on Escape, or explicitly,
// never leak a press in the first place and don't need one.
Popup {
    id: shield

    // The menu/dialog whose visibility this shield follows.
    property T.Popup host: null

    parent: Overlay.overlay
    z: host ? host.z - 0.5 : -1
    modal: true
    dim: false
    closePolicy: Popup.NoAutoClose
    padding: 0
    x: 0; y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    background: null

    // Closing a beat after the host — rather than in lockstep with it — keeps
    // the overlay blocking until the press that dismissed the host has finished
    // being delivered; closing synchronously would re-open the very gap this
    // component exists to cover.
    Timer { id: closeDelay; interval: 1; onTriggered: shield.close() }

    Connections {
        target: shield.host
        function onVisibleChanged() {
            if (shield.host.visible) {
                closeDelay.stop()
                shield.open()
            } else {
                closeDelay.restart()
            }
        }
    }
}

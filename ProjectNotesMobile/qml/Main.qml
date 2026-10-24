// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.iOS
import QtQuick.Effects
import QtQuick.Layouts
import ProjectNotesMobile

ApplicationWindow {
    id: root
    visible: true
    width: 390
    height: 844
    title: qsTr("Project Notes")

    // Since Qt 6.9 the window pads its content by the safe-area margins, which
    // left a band of window background under the lists. Drop the bottom
    // padding so the tab lists run to the screen edge under the floating tab
    // bar; pageStack re-applies the inset for detail pages, whose footer
    // toolbars must stay clear of the home indicator.
    bottomPadding: 0
    readonly property real safeBottom: SafeArea.margins.bottom
    // Likewise at the top: the pages run up under the status bar and the
    // frosted title bar, which pads for the safe area itself.
    topPadding: 0

    // Pages, forms and lists share one background; see Theme.pageBackground.
    palette.window: Theme.pageBackground

    // Monochrome buttons in place of the iOS style's system blue. Flat buttons
    // draw their text in palette.button; filled ones fill with it and draw
    // their text in buttonText, so that is the page color. Each group is set
    // on its own so disabled buttons keep a dimmed tint. palette.highlight is
    // left alone: text selection and the sheets' checkmarks use it too.
    palette.active.button:       Theme.iconColor
    palette.inactive.button:     Theme.iconColor
    palette.disabled.button:     Theme.iconColorDisabled
    palette.buttonText:          Theme.pageBackground

    // The default font every control inherits (most list rows show text at
    // this size), scaled by the Text Size setting, with SF Pro's native
    // tracking. Controls that set their own font.pixelSize use Theme's sizes,
    // which scale the same way, and set a matching font.letterSpacing.
    readonly property int _baseFontSize: Qt.application.font.pixelSize > 0
                                         ? Qt.application.font.pixelSize
                                         : Qt.application.font.pointSize
    font.pixelSize: Theme.scaled(_baseFontSize)
    font.letterSpacing: Theme.tracking(Theme.scaled(_baseFontSize))

    // Text Size steps (Appearance page) as a multiple of the default body
    // size: iOS Dynamic Type's body sizes 14-23pt over its 17pt default.
    readonly property var _textScales: [14/17, 15/17, 16/17, 1.0, 19/17, 21/17, 23/17]
    Binding { target: Theme; property: "textScale"; value: root._textScales[AppController.textSize] || 1.0 }

    // Save current page data before app closes
    function saveCurrentPage() {
        var page = pageStack.currentItem
        // Cloud Sync Settings isn't a record editor — it just needs its typed
        // fields committed. No credential check here: leaving the app is not
        // the moment to put a dialog in the user's way.
        if (page && typeof page._commitSyncFields === "function") {
            page._commitSyncFields()
            return
        }
        if (page && typeof page._saveNow === "function" && !page._skipSave) {
            if (page.isNewRecord && typeof page._isBlankNew === "function" && page._isBlankNew()) {
                page._skipSave = true
                if (typeof page._discardNew === "function")
                    page._discardNew()
                return
            }
            page._saveNow()
        }
    }

    // Save on app close or background
    Component.onDestruction: saveCurrentPage()

    Connections {
        target: Qt.application
        function onStateChanged(state) {
            if (state === Qt.ApplicationInactive || state === Qt.ApplicationSuspended)
                saveCurrentPage()
        }
    }

    // Shared tab state — lives here so the TabBar (footer) and SwipeView
    // (inside the StackView initialItem component) can both bind to it.
    property int currentTabIndex: 0
    readonly property var tabTitles: [
        qsTr("Projects"), qsTr("Contacts"), qsTr("Companies"), qsTr("Items")
    ]

    // ── Leaving Cloud Sync Settings: check the fields first ───────────────────
    // The sync credentials and the encryption phrase are write-through — a
    // mistyped password or a phrase that doesn't match the account is simply
    // saved, and only shows up much later as a sync that never completes. So
    // when the user leaves that page having edited any of them, verify them
    // against the host and say so while they can still fix it. The pop waiting
    // on that answer is parked here. Mirrors the desktop app's Settings gate.
    property var _pendingNavigation: null

    // Returns true when `next` has been parked pending the check — the caller
    // must then do nothing; _resumeNavigation() runs it once the user is done
    // with the dialog.
    function _gateOnSyncCheck(next) {
        if (!AppController.syncSettingsUnverified)
            return false
        root._pendingNavigation = next
        syncCheckDialog.status = ""
        AppController.verifySyncSettings()
        // With sync off or unconfigured there is nothing to contact the host
        // about: the verdict came back inside that call and already ran the
        // navigation, so don't flash a dialog for it.
        if (root._pendingNavigation !== null)
            syncCheckDialog.open()
        return true
    }

    function _resumeNavigation() {
        var next = root._pendingNavigation
        root._pendingNavigation = null
        if (next)
            next()
    }

    // Called by NavButton on the tab pages.
    function openMainMenu() {
        pageStack.push(Qt.resolvedUrl("pages/MainMenuPage.qml"), { stackView: pageStack })
    }

    // Attempt to save the current detail page and pop.  If the C++ layer
    // rejects the data (lastSaveError is non-empty), stay on the page so the
    // user can fix their input — the errorOccurred signal opens the dialog,
    // and the typed values stay visible so it's clear which field is wrong.
    function trySaveAndPop() {
        var page = pageStack.currentItem
        // Cloud Sync Settings: commit the fields, then check them before the
        // page goes away. It has no record to save, so it never reaches the
        // detail-page path below.
        if (page && typeof page._commitSyncFields === "function") {
            page._commitSyncFields()
            if (_gateOnSyncCheck(function() { pageStack.pop() }))
                return
            pageStack.pop()
            return
        }
        if (page && typeof page._saveNow === "function" && !page._skipSave) {
            if (page.isNewRecord && typeof page._isBlankNew === "function" && page._isBlankNew()) {
                page._skipSave = true
                if (typeof page._discardNew === "function")
                    page._discardNew()
                pageStack.pop()
                return
            }
            page._saveNow()
            if (AppController.lastSaveError() !== "") {
                return  // validation error — stay on page with typed values intact
            }
            page._skipSave = true  // prevent double-save from onDeactivating / onDestruction
        }
        pageStack.pop()
    }

    // ── C++ signal connections ────────────────────────────────────────────────
    Connections {
        target: AppController
        function onErrorOccurred(title, message) {
            errorDialog.title   = title
            errorDialog.message = message
            errorDialog.open()
        }
        function onDatabaseReady() {
            // Filters are already applied by openOrCreateDatabase() before this
            // signal fires — no override needed here.
        }
        function onSubscriptionExpired() {
            subscriptionExpiredDialog.open()
        }
        // Verdict on the cloud sync settings the user just edited (see
        // _gateOnSyncCheck). A late answer for a check the user already skipped
        // has no pop parked against it and is theirs to ignore.
        function onSyncSettingsVerified(status, message) {
            if (root._pendingNavigation === null)
                return
            if (message === "") {
                // Nothing to report — carry on and leave the page.
                root._resumeNavigation()
                syncCheckDialog.close()
                return
            }
            syncCheckDialog.message = message
            syncCheckDialog.status = status
        }
    }

    Dialog {
        id: errorDialog
        property string message: ""
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok
        palette.highlight: Theme.iconColorPressed   // monochrome pressed OK button
        Label { text: errorDialog.message; wrapMode: Text.Wrap; width: 260 }
    }

    Dialog {
        id: subscriptionExpiredDialog
        title: qsTr("Subscription Expired")
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok
        palette.highlight: Theme.iconColorPressed   // monochrome pressed OK button

        Label {
            width: 260
            wrapMode: Text.Wrap
            textFormat: Text.RichText
            text: qsTr("Your Project Notes Pro subscription has expired.<br><br>"
                      + "The application will no longer sync changes between your devices, "
                      + "and your data is no longer backed up to the Project Notes Pro server.<br><br>"
                      + "To re-enable sync, please visit "
                      + "<a href=\"https://www.projectnotespro.com\">www.projectnotespro.com</a> "
                      + "to renew your subscription.")
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }

    // Cloud sync check, shown on the way out of Cloud Sync Settings after the
    // user edits the credentials or the encryption phrase (see
    // _gateOnSyncCheck). An empty status means the check is still running;
    // anything else is a problem worth stopping for.
    Dialog {
        id: syncCheckDialog
        property string status: ""
        property string message: ""
        readonly property bool checking: status === ""
        // "Back to Settings" is the only outcome that keeps the user on the
        // page, so credential/phrase problems are the only ones with a choice.
        readonly property bool fixable: status === "credentials" || status === "encryption"

        title: checking ? qsTr("Checking Cloud Sync") : qsTr("Cloud Sync")
        modal: true
        anchors.centerIn: Overlay.overlay
        width: Math.min(320, root.width - 48)
        closePolicy: checking ? Popup.NoAutoClose : Popup.CloseOnEscape
        footer: null

        // Dismissing the dialog any other way means "stay here and fix it", so
        // drop the pop that was waiting on it. The buttons that do continue run
        // the pop before closing.
        onClosed: root._pendingNavigation = null

        ColumnLayout {
            width: parent.width
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                visible: syncCheckDialog.checking
                spacing: 10
                BusyIndicator {
                    running: syncCheckDialog.visible && syncCheckDialog.checking
                    implicitWidth: 28
                    implicitHeight: 28
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("Checking your sync email, password, and encryption phrase…")
                }
            }

            Label {
                Layout.fillWidth: true
                visible: !syncCheckDialog.checking
                wrapMode: Text.Wrap
                text: syncCheckDialog.message
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }

                // Leave with the settings as typed — they stay saved either way.
                MonoButton {
                    text: qsTr("Leave Anyway")
                    visible: syncCheckDialog.fixable
                    flat: true
                    onClicked: { root._resumeNavigation(); syncCheckDialog.close() }
                }

                MonoButton {
                    text: syncCheckDialog.checking ? qsTr("Skip")
                          : (syncCheckDialog.fixable ? qsTr("Back to Settings") : qsTr("OK"))
                    flat: true
                    onClicked: {
                        // Skip and OK carry on and leave the page; Back to
                        // Settings keeps the user here.
                        if (!syncCheckDialog.fixable)
                            root._resumeNavigation()
                        syncCheckDialog.close()
                    }
                }
            }
        }
    }

    // ── Title bar ─────────────────────────────────────────────────────────────
    // Frosted glass over the top of the page: the pages run up underneath it
    // (they start their own rows Theme.topInset down) and show through
    // blurred as they scroll, as in iOS. Covers the status bar too.
    Item {
        id: titleBar
        z: 15
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: SafeArea.margins.top + 52

        // A live copy of whatever is behind the bar, blurred. The opaque base
        // keeps the sharp original from showing through the blur.
        Rectangle {
            anchors.fill: parent
            color: Theme.pageBackground
        }
        ShaderEffectSource {
            id: titleBackdrop
            anchors.fill: parent
            sourceItem: pageStack
            sourceRect: Qt.rect(0, titleBar.y - pageStack.y, titleBar.width, titleBar.height)
            live: true
            visible: false
        }
        MultiEffect {
            anchors.fill: parent
            source: titleBackdrop
            autoPaddingEnabled: false
            blurEnabled: true
            blur: 1.0
            blurMax: 48
            saturation: 0.3
        }
        // Tint, so the title reads over anything, and a hairline underneath.
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(Theme.pageBackground.r, Theme.pageBackground.g, Theme.pageBackground.b, 0.72)
        }
        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 1
            color: Theme.separator
        }

        Label {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                      leftMargin: 16; rightMargin: 16 }
            height: 52
            text: pageStack.depth > 1
                ? (pageStack.currentItem ? (pageStack.currentItem.title || "") : "")
                : (root.tabTitles[root.currentTabIndex] || "")
            font.pixelSize: Theme.headline; font.letterSpacing: Theme.tracking(Theme.headline)
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            color: palette.windowText
        }
    }

    // Pages offset their top row by this much to clear the title bar.
    Binding { target: Theme; property: "topInset"; value: titleBar.height }

    // ── Sync progress strip ───────────────────────────────────────────────────
    // Sits just under the title bar, over the page.
    // Green while syncing, red on error; auto-hides 2 s after successful completion.
    Rectangle {
        id: syncStrip
        z: 16
        anchors { top: titleBar.bottom; left: parent.left; right: parent.right }
        height: AppController.syncProgress >= 0.0 ? 5 : 0
        color: palette.window        // strip background (shows when bar < 100%)

        Behavior on height { NumberAnimation { duration: 150 } }

        Rectangle {
            anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
            width: parent.width * Math.max(0.0, Math.min(1.0, AppController.syncProgress))
            color: AppController.syncHasError ? "#cc2222" : Theme.accentGreen  // red / iOS green
            Behavior on width { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }
        }
    }

    // ── Main content ──────────────────────────────────────────────────────────
    // The StackView's initialItem is the tab content (SwipeView).  Pages pushed
    // from the main menu land on top at depth 2+.  pop() back to depth 1 is always
    // valid — Qt forbids pop() that would empty the stack entirely.
    StackView {
        id: pageStack
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: parent.bottom }
        anchors.bottomMargin: depth > 1 ? root.safeBottom : 0
        initialItem: mainTabsComponent
    }

    Component {
        id: mainTabsComponent

        SwipeView {
            id: swipeView
            currentIndex: root.currentTabIndex
            onCurrentIndexChanged: {
                root.currentTabIndex = currentIndex
                switch (currentIndex) {
                    case 0: AppController.refreshProjectsList(); break
                    case 1: AppController.refreshContacts();     break
                    case 2: AppController.refreshCompanies();    break
                    case 3: AppController.refreshAllItems();     break
                }
            }

            Connections {
                target: AppController
                function onViewOptionsChanged() {
                    if (pageStack.depth > 1) return   // a detail page is stacked on top
                    switch (swipeView.currentIndex) {
                        case 0: AppController.refreshProjectsList(); break
                        case 1: AppController.refreshContacts();     break
                        case 2: AppController.refreshCompanies();    break
                        case 3: AppController.refreshAllItems();     break
                    }
                }
            }

            ProjectsListPage { stackView: pageStack; listBottomInset: root.tabDockInset }
            ContactsPage      { stackView: pageStack; listBottomInset: root.tabDockInset }
            CompaniesPage     { stackView: pageStack; listBottomInset: root.tabDockInset }
            AllItemsPage      { stackView: pageStack; listBottomInset: root.tabDockInset }
        }
    }

    // ── Bottom tab bar ────────────────────────────────────────────────────────
    // A floating glass capsule over the list, in the style of iOS 26 tab bars.
    // The lists run full height underneath it; each tab page pads its list by
    // tabDockInset so the last row can still scroll clear of the capsule.
    readonly property real tabDockInset: tabDock.height + tabDock.anchors.bottomMargin

    GlassBar {
        id: tabDock
        z: 10
        // Hidden while a detail page is open, as the old footer was.
        visible: pageStack.depth <= 1
        height: 62
        anchors {
            left: parent.left; right: parent.right; bottom: parent.bottom
            leftMargin: 16; rightMargin: 16
            // Sit just above the home indicator, or a fixed gap without one.
            bottomMargin: Math.max(12, root.safeBottom - 10)
        }

        blurSource: pageStack
        blurRect: Qt.rect(tabDock.x - pageStack.x, tabDock.y - pageStack.y,
                          tabDock.width, tabDock.height)

        TabBar {
            id: tabBar
            anchors.fill: parent
            position: TabBar.Footer
            currentIndex: root.currentTabIndex
            onCurrentIndexChanged: root.currentTabIndex = currentIndex

            // The iOS style tints the selected tab with palette.button (system
            // blue); use the monochrome icon color instead. TabButtons inherit it.
            palette.button: Theme.iconColor

            // No bar background; just a pill that slides behind the current tab.
            background: Item {
                Rectangle {
                    width: tabBar.count > 0 ? tabBar.width / tabBar.count : 0
                    height: tabBar.height
                    x: tabBar.currentIndex * width
                    radius: height / 2
                    color: Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.06)
                    Behavior on x { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                }
            }

            TabButton { text: qsTr("Projects"); icon.name: "folder"    }
            TabButton { text: qsTr("Contacts"); icon.name: "person"    }
            TabButton { text: qsTr("Companies"); icon.name: "building"  }
            TabButton { text: qsTr("Items");    icon.name: "list.bullet" }
        }
    }

    // ── Startup ───────────────────────────────────────────────────────────────
    // Open the database synchronously, directly in Component.onCompleted —
    // matching ProjectNotesDesktop's Main.qml construction exactly. This runs
    // during engine.load(), before main.cpp's window->show(), so the DB open,
    // initial model refresh(), and column-filter restore below all complete
    // before any ListView delegate exists. Deferring the whole call via
    // Qt.callLater() (the original approach, kept the tab chrome from flashing
    // blank while SQL work ran) let the SwipeView's ListViews render and bind
    // to these exact models FIRST, so that work ran against live-viewed
    // proxies instead — unsafe for the same reason described in
    // AppController::openOrCreateDatabase()'s sort comment. Desktop never
    // defers this call and never hits it. iOS's own LaunchScreen storyboard
    // already covers the launch gap, so there's no bare black-screen risk.
    //
    // Sort restoration is the one piece of openOrCreateDatabase() that is NOT
    // synchronous with the rest of this — see restoreAllSorts() and the
    // QTimer::singleShot() call in AppController::openOrCreateDatabase() for
    // why it's deferred to its own later event-loop turn instead.
    Component.onCompleted: {
        AppController.openOrCreateDatabase()
        if (AppController.syncEnabled)
            AppController.startSync()
    }
}

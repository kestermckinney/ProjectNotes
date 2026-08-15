// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.iOS
import QtQuick.Layouts
import ProjectNotesMobile

ApplicationWindow {
    id: root
    visible: true
    width: 390
    height: 844
    title: qsTr("Project Notes")

    // Save current page data before app closes
    function saveCurrentPage() {
        var page = pageStack.currentItem
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
        qsTr("Projects"), qsTr("People"), qsTr("Clients"), qsTr("Items")
    ]

    // Attempt to save the current detail page and pop.  If the C++ layer
    // rejects the data (lastSaveError is non-empty), stay on the page so the
    // user can fix their input — the errorOccurred signal opens the dialog,
    // and the typed values stay visible so it's clear which field is wrong.
    function trySaveAndPop() {
        var page = pageStack.currentItem
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
    }

    Dialog {
        id: errorDialog
        property string message: ""
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok
        Label { text: errorDialog.message; wrapMode: Text.Wrap; width: 260 }
    }

    Dialog {
        id: subscriptionExpiredDialog
        title: qsTr("Subscription Expired")
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok

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

    // ── Hamburger drawer ──────────────────────────────────────────────────────
    Drawer {
        id: hamburgerDrawer
        width: Math.min(280, root.width * 0.78)
        height: root.height
        edge: Qt.LeftEdge

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                height: 32
                color: Theme.navyDark

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Main Menu")
                    font.pixelSize: 20
                    font.bold: true
                    color: "white"
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth

                Column {
                    width: parent.width

                    ItemDelegate {
                        width: parent.width
                        text: qsTr("Cloud Sync Settings")
                        onClicked: {
                            hamburgerDrawer.close()
                            pageStack.push(Qt.resolvedUrl("pages/SyncSettingsPage.qml"))
                        }
                    }

                    MenuSeparator { width: parent.width }

                    ItemDelegate {
                        width: parent.width
                        text: qsTr("View")
                        onClicked: {
                            hamburgerDrawer.close()
                            pageStack.push(Qt.resolvedUrl("pages/ViewOptionsPage.qml"))
                        }
                    }

                    ItemDelegate {
                        width: parent.width
                        text: qsTr("Sync All…")
                        enabled: AppController.syncEnabled
                        onClicked: {
                            hamburgerDrawer.close()
                            AppController.syncAll()
                        }
                    }

                    MenuSeparator { width: parent.width }

                    ItemDelegate {
                        width: parent.width
                        text: qsTr("Preferences…")
                        onClicked: {
                            hamburgerDrawer.close()
                            pageStack.push(Qt.resolvedUrl("pages/PreferencesPage.qml"))
                        }
                    }

                    MenuSeparator { width: parent.width }

                    ItemDelegate {
                        width: parent.width
                        text: qsTr("Help")
                        onClicked: {
                            hamburgerDrawer.close()
                            Qt.openUrlExternally("https://projectnotes.readthedocs.io/en/latest/Mobile/ProjectNotesMobile/")
                        }
                    }

                    ItemDelegate {
                        width: parent.width
                        text: qsTr("What's New")
                        onClicked: {
                            hamburgerDrawer.close()
                            Qt.openUrlExternally("https://github.com/kestermckinney/ProjectNotes/releases")
                        }
                    }

                    ItemDelegate {
                        width: parent.width
                        text: qsTr("About")
                        onClicked: {
                            hamburgerDrawer.close()
                            pageStack.push(Qt.resolvedUrl("pages/AboutPage.qml"))
                        }
                    }
                }
            }
        }
    }

    // ── Persistent header toolbar ─────────────────────────────────────────────
    header: ToolBar {
        // palette.window drives the iOS-style ToolBar tint; background overrides the QML layer
        palette.window: Theme.navyDark
        background: Rectangle { color: Theme.navyDark }
        height: 100

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 8

            ToolButton {
                id: navButton
                // depth 1 = initial SwipeView item; depth > 1 = a detail page is open
                icon.name: pageStack.depth > 1 ? "chevron.left" : "line.3.horizontal"
                icon.color: "white"
                onClicked: {
                    if (pageStack.depth > 1)
                        trySaveAndPop()
                    else
                        hamburgerDrawer.open()
                }
            }

            Label {
                Layout.fillWidth: true
                text: pageStack.depth > 1
                    ? (pageStack.currentItem ? (pageStack.currentItem.title || "") : "")
                    : (root.tabTitles[root.currentTabIndex] || "")
                font.pixelSize: 17
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                color: "white"
            }

            Item { width: navButton.width }
        }
    }

    // ── Sync progress strip ───────────────────────────────────────────────────
    // Sits at the very top of the content area (below the header toolbar).
    // Green while syncing, red on error; auto-hides 2 s after successful completion.
    Rectangle {
        id: syncStrip
        z: 10
        anchors { top: parent.top; left: parent.left; right: parent.right }
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
    // from the drawer land on top at depth 2+.  pop() back to depth 1 is always
    // valid — Qt forbids pop() that would empty the stack entirely.
    StackView {
        id: pageStack
        anchors { top: syncStrip.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
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
                    case 1: AppController.refreshPeople();       break
                    case 2: AppController.refreshClients();      break
                    case 3: AppController.refreshAllItems();     break
                }
            }

            Connections {
                target: AppController
                function onViewOptionsChanged() {
                    if (pageStack.depth > 1) return   // a detail page is stacked on top
                    switch (swipeView.currentIndex) {
                        case 0: AppController.refreshProjectsList(); break
                        case 1: AppController.refreshPeople();       break
                        case 2: AppController.refreshClients();      break
                        case 3: AppController.refreshAllItems();     break
                    }
                }
            }

            ProjectsListPage { stackView: pageStack }
            PeoplePage        { stackView: pageStack }
            ClientsPage       { stackView: pageStack }
            AllItemsPage      { stackView: pageStack }
        }
    }

    // ── Bottom tab bar ────────────────────────────────────────────────────────
    footer: TabBar {
        id: tabBar
        // Collapse when a detail page is open so StackView fills the full window.
        visible: pageStack.depth <= 1
        height: visible ? implicitHeight : 0
        currentIndex: root.currentTabIndex
        onCurrentIndexChanged: root.currentTabIndex = currentIndex

        TabButton { text: qsTr("Projects"); icon.name: "folder"    }
        TabButton { text: qsTr("People");   icon.name: "person"    }
        TabButton { text: qsTr("Clients");  icon.name: "building"  }
        TabButton { text: qsTr("Items");    icon.name: "list.bullet" }
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

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

    // Shared tab state — lives here so the TabBar (footer) and SwipeView
    // (inside the StackView initialItem component) can both bind to it.
    property int currentTabIndex: 0
    readonly property var tabTitles: [
        qsTr("Projects"), qsTr("People"), qsTr("Clients"), qsTr("Items")
    ]

    // Attempt to save the current detail page and pop.  If the C++ layer
    // rejects the data (lastSaveError is non-empty), reload the page to show
    // the original valid values and stay — the errorOccurred signal will open
    // the error dialog asynchronously.
    function trySaveAndPop() {
        var page = pageStack.currentItem
        if (page && typeof page._saveNow === "function" && !page._skipSave) {
            page._saveNow()
            if (AppController.lastSaveError() !== "") {
                if (typeof page._reloadData === "function")
                    page._reloadData()
                return  // validation error — stay on page, dialog opens via signal
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
    }

    Dialog {
        id: errorDialog
        property string message: ""
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok
        Label { text: errorDialog.message; wrapMode: Text.Wrap; width: 260 }
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
                height: 110
                color: Theme.navyDark

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 52; height: 52
                        radius: 12
                        color: "white"

                        Label {
                            anchors.centerIn: parent
                            text: "PN"
                            font.pixelSize: 20
                            font.bold: true
                            color: Theme.navyDark
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Project Notes")
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                    }
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
                if (currentIndex === 3)
                    AppController.refreshAllItems()
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
    // Defer DB init so the QML shell renders its first frame before the
    // synchronous SQL work begins — eliminates the black-screen delay.
    Component.onCompleted: Qt.callLater(function() {
        AppController.openOrCreateDatabase()
        if (AppController.syncEnabled)
            AppController.startSync()
    })
}

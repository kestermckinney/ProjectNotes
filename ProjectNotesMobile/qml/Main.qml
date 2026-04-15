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
                height: 80
                color: palette.button

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Project Notes")
                    font.pixelSize: 18
                    font.bold: true
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
                            Qt.openUrlExternally("https://github.com/kestermckinney/ProjectNotes/wiki/Release%20Notes")
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
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 8

            ToolButton {
                id: navButton
                // depth 1 = initial SwipeView item; depth > 1 = a detail page is open
                text: pageStack.depth > 1 ? "‹" : "≡"
                font.pixelSize: 29
                onClicked: {
                    if (pageStack.depth > 1)
                        pageStack.pop()
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
        height: AppController.syncProgress >= 0.0 ? 3 : 0
        color: palette.window        // strip background (shows when bar < 100%)

        Behavior on height { NumberAnimation { duration: 150 } }

        Rectangle {
            anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
            width: parent.width * Math.max(0.0, Math.min(1.0, AppController.syncProgress))
            color: AppController.syncHasError ? "#cc2222" : "#34c759"  // red / iOS green
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
            onCurrentIndexChanged: root.currentTabIndex = currentIndex

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
    Component.onCompleted: {
        AppController.openOrCreateDatabase()
        if (AppController.syncEnabled)
            AppController.startSync()
    }
}

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Full-screen main menu, opened from the header's ≡ button. It sits on the
// page stack like any other page, so the header shows the back chevron and
// the menu's own settings pages push on top of it and come back to it.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Main Menu")

    property StackView stackView: null

    function _push(file) {
        if (root.stackView)
            root.stackView.push(Qt.resolvedUrl(file))
    }

    // Same spot as on the list pages, where it is centered on the 48px search
    // capsule that sits 6px from the top.
    NavButton {
        id: navButton
        anchors { top: parent.top; left: parent.left; topMargin: 8; leftMargin: 12 }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        Column {
            width: parent.width
            // The rows start below the back button, as list rows start below
            // the search capsule.
            topPadding: navButton.y + navButton.height + 12

            MenuRow {
                text: qsTr("Cloud Sync Settings")
                onClicked: root._push("SyncSettingsPage.qml")
            }

            MenuRow {
                text: qsTr("View")
                onClicked: root._push("ViewOptionsPage.qml")
            }

            // Back to the lists so the sync strip shows over what is syncing.
            MenuRow {
                text: qsTr("Sync All…")
                showChevron: false
                enabled: AppController.syncEnabled
                onClicked: {
                    if (root.stackView)
                        root.stackView.pop()
                    AppController.syncAll()
                }
            }

            MenuRow {
                text: qsTr("Preferences…")
                onClicked: root._push("PreferencesPage.qml")
            }

            MenuRow {
                text: qsTr("Help")
                showChevron: false
                onClicked: Qt.openUrlExternally("https://projectnotes.readthedocs.io/en/latest/Mobile/ProjectNotesMobile/")
            }

            MenuRow {
                text: qsTr("What's New")
                showChevron: false
                onClicked: Qt.openUrlExternally("https://gitlab.com/kestermckinney/ProjectNotes/-/releases")
            }

            MenuRow {
                text: qsTr("About")
                onClicked: root._push("AboutPage.qml")
            }
        }
    }

    // A full-width row; rows that open another page show a chevron, as in
    // the iOS Settings app.
    component MenuRow: ItemDelegate {
        property bool showChevron: true

        width: parent ? parent.width : implicitWidth
        font.pixelSize: Theme.body; font.letterSpacing: Theme.tracking(Theme.body)
        rightPadding: showChevron ? 40 : 16

        Label {
            visible: parent.showChevron
            anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
            text: "›"
            font.pixelSize: Theme.title3
            color: Theme.mutedText
            opacity: 0.5
        }
    }
}

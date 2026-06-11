// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("About")

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Project Notes")
            font.pixelSize: 26
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Version 5.2.0")
            font.pixelSize: 15
            color: Theme.mutedText
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("© 2022–2026 Paul McKinney")
            font.pixelSize: 13
            color: Theme.mutedText
        }

        Item { height: 16 }

        Button {
            Layout.alignment: Qt.AlignHCenter
            highlighted: true
            text: qsTr("Documentation")
            onClicked: Qt.openUrlExternally("https://projectnotes.readthedocs.io/")
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Release Notes")
            onClicked: Qt.openUrlExternally("https://github.com/kestermckinney/ProjectNotes/releases")
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Source Code")
            onClicked: Qt.openUrlExternally("https://github.com/kestermckinney/ProjectNotes")
        }

        Item { height: 8 }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: root.width * 0.85
            implicitHeight: promoColumn.implicitHeight + 24
            color: Theme.sectionBg
            radius: 10
            border.color: Theme.navyMid
            border.width: 1

            ColumnLayout {
                id: promoColumn
                anchors { top: parent.top; left: parent.left; right: parent.right; margins: 12 }
                spacing: 6

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Get the Full Experience")
                    font.pixelSize: 15
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Download the Project Notes desktop application for Windows or macOS to unlock the full suite of project management features — plugins, Outlook integration and advanced reporting.")
                    font.pixelSize: 13
                    color: Theme.mutedText
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "<a href=\"https://www.projectnotespro.com\">www.projectnotespro.com</a>"
            textFormat: Text.RichText
            font.pixelSize: 15
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
}

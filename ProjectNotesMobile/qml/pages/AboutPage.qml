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

        // ── App icon ──────────────────────────────────────────────────────────
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 80; height: 80
            radius: 18
            color: Theme.navyMid

            Label {
                anchors.centerIn: parent
                text: "PN"
                font.pixelSize: 30
                font.bold: true
                color: "white"
            }
        }

        Item { height: 4 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Project Notes")
            font.pixelSize: 26
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Version 5.0.0")
            font.pixelSize: 15
            color: palette.placeholderText
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("© 2022–2026 Paul McKinney")
            font.pixelSize: 13
            color: palette.placeholderText
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
    }
}

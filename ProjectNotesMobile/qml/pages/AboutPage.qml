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
            text: qsTr("Version 5.0.0")
            font.pixelSize: 15
            color: palette.mid
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("© 2022–2026 Paul McKinney")
            font.pixelSize: 13
            color: palette.mid
        }

        Item { height: 12 }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Documentation")
            onClicked: Qt.openUrlExternally("https://projectnotes.readthedocs.io/")
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Release Notes")
            onClicked: Qt.openUrlExternally("https://github.com/kestermckinney/ProjectNotes/wiki/Release%20Notes")
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Source Code")
            onClicked: Qt.openUrlExternally("https://github.com/kestermckinney/ProjectNotes")
        }
    }
}

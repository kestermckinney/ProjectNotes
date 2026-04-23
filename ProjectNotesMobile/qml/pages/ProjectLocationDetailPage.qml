// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("File / Folder")

    property int    locationRow:        -1
    property string initialType:        ""
    property string initialDescription: ""
    property string initialPath:        ""
    property bool   _skipSave:          false

    function _saveNow() {
        var locType = (typeCombo.currentIndex >= 0)
            ? typeCombo.model[typeCombo.currentIndex] : ""
        AppController.saveProjectLocation(root.locationRow, locType, descField.text, pathField.text)
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    Component.onDestruction: {
        root.forceActiveFocus()
        Qt.inputMethod.hide()
        if (!root._skipSave)
            root._saveNow()
    }

    // ── Toolbar: copy + delete ────────────────────────────────────────────────
    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            Item { Layout.fillWidth: true }

            ToolButton {
                icon.name: "doc.on.doc"
                onClicked: {
                    root._saveNow()
                    root._skipSave = true
                    var newRow = AppController.copyProjectLocation(root.locationRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getProjectLocationData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("ProjectLocationDetailPage.qml"), {
                        locationRow:         newRow,
                        initialType:         (d.location_type        || "").toString(),
                        initialDescription:  (d.location_description || "").toString(),
                        initialPath:         (d.full_path            || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deleteProjectLocation(root.locationRow)
                    root.StackView.view.pop()
                }
            }
        }
    }

    // ── Footer: open web link ─────────────────────────────────────────────────
    footer: ToolBar {
        visible: pathField.text.startsWith("http://") || pathField.text.startsWith("https://")
                 || typeCombo.currentIndex >= 0 && typeCombo.model[typeCombo.currentIndex] === "Web Link"
        RowLayout {
            anchors.centerIn: parent
            ToolButton {
                icon.name: "safari"
                text: qsTr("Open in Browser")
                display: AbstractButton.TextUnderIcon
                onClicked: Qt.openUrlExternally(pathField.text)
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            SectionHeader { text: qsTr("Type") }
            FieldRow {
                ComboBox {
                    id: typeCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.fileTypeOptions()
                    Component.onCompleted: {
                        var idx = model.indexOf(root.initialType)
                        currentIndex = (idx >= 0) ? idx : 0
                    }
                }
            }

            SectionHeader { text: qsTr("Description") }
            FieldRow {
                TextField {
                    id: descField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialDescription
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Path / URL") }
            FieldRow {
                TextField {
                    id: pathField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialPath
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    component SectionHeader: Label {
        Layout.fillWidth: true
        Layout.topMargin: 20
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: 13
        font.weight: 600
        color: Theme.navyMid
        background: Rectangle { color: Theme.sectionBg }
    }

    component FieldRow: Rectangle {
        default property alias content: innerItem.data
        Layout.fillWidth: true
        Layout.preferredHeight: 44
        color: palette.base
        Item { id: innerItem; anchors.fill: parent }
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
            height: 1; color: palette.placeholderText; opacity: 0.3
        }
    }
}

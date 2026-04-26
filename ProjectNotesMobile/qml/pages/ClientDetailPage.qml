// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Client")

    property int    clientRow:         -1
    property string initialClientName: ""
    property bool   _skipSave:         false

    function _saveNow() {
        return AppController.saveClient(root.clientRow, clientNameField.text)
    }

    function _reloadData() {
        var d = AppController.getClientData(root.clientRow)
        clientNameField.text = (d.client_name || "").toString()
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
                    if (!root._saveNow()) return
                    root._skipSave = true
                    var newRow = AppController.copyClient(root.clientRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getClientData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("ClientDetailPage.qml"), {
                        clientRow:         newRow,
                        initialClientName: (d.client_name || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deleteClient(root.clientRow)
                    root.StackView.view.pop()
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            SectionHeader { text: qsTr("Client Name") }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: palette.base

                TextField {
                    id: clientNameField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialClientName
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }

                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1
                    color: palette.placeholderText
                    opacity: 0.3
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    // ── Shared helper components ──────────────────────────────────────────────

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


}

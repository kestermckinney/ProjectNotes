// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("Person")

    property int    personRow:          -1
    property string initialName:        ""
    property string initialEmail:       ""
    property string initialOfficePhone: ""
    property string initialCellPhone:   ""
    property string initialClientId:    ""
    property string initialRole:        ""
    property bool   _skipSave:          false

    function _saveNow() {
        var clientId = (clientCombo.currentIndex >= 0)
            ? AppController.clientIdAtRow(clientCombo.currentIndex) : ""
        AppController.savePerson(root.personRow, nameField.text, emailField.text,
                                 officePhoneField.text, cellPhoneField.text,
                                 clientId, roleField.text)
    }

    StackView.onDeactivating: {
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
                    var newRow = AppController.copyPerson(root.personRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getPersonData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("PersonDetailPage.qml"), {
                        personRow:          newRow,
                        initialName:        (d.name         || "").toString(),
                        initialEmail:       (d.email        || "").toString(),
                        initialOfficePhone: (d.office_phone || "").toString(),
                        initialCellPhone:   (d.cell_phone   || "").toString(),
                        initialClientId:    (d.client_id    || "").toString(),
                        initialRole:        (d.role         || "").toString()
                    })
                }
            }

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deletePerson(root.personRow)
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

            // ── Contact ───────────────────────────────────────────────────────

            SectionHeader { text: qsTr("Name") }
            FieldRow {
                TextField {
                    id: nameField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialName
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Email") }
            FieldRow {
                TextField {
                    id: emailField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialEmail
                    inputMethodHints: Qt.ImhEmailCharactersOnly
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Office Phone") }
            FieldRow {
                TextField {
                    id: officePhoneField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialOfficePhone
                    inputMethodHints: Qt.ImhDialableCharactersOnly
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Cell Phone") }
            FieldRow {
                TextField {
                    id: cellPhoneField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialCellPhone
                    inputMethodHints: Qt.ImhDialableCharactersOnly
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Role") }
            FieldRow {
                TextField {
                    id: roleField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialRole
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            // ── Company ───────────────────────────────────────────────────────

            SectionHeader { text: qsTr("Client") }
            FieldRow {
                ComboBox {
                    id: clientCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.clientsModel
                    textRole: "client_name"
                    Component.onCompleted: {
                        var row = AppController.clientRowForId(root.initialClientId)
                        currentIndex = (row >= 0) ? row : -1
                    }
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
        font.weight: Font.Medium
        color: palette.mid
        background: Rectangle { color: palette.window }
    }

    component FieldRow: Rectangle {
        default property alias content: innerItem.data

        Layout.fillWidth: true
        Layout.preferredHeight: 44
        color: palette.base

        Item {
            id: innerItem
            anchors.fill: parent
        }

        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
            height: 1
            color: palette.mid
            opacity: 0.3
        }
    }
}

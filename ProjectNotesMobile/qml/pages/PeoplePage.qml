// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("People")

    property StackView stackView: null

    // Quick Filter entry for a long-pressed person row: pre-filled from that
    // row's own client — client_id is a genuine direct column on the people
    // table (peoplemodel.cpp), not derived.
    function _quickFiltersForRow(m) {
        var qf = []
        var clientId = (m.client_id || "").toString()
        if (clientId !== "")
            qf.push({ label: qsTr("This Client"), field: "client_id", values: [clientId] })
        return qf
    }

    FilterSheet     { id: filterSheet }
    SortSheet       { id: sortSheet }
    QuickFilterDialog { id: qfDialog }

    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search people…")
                onTextChanged: AppController.setQuickSearch(AppController.peopleModel, text)
                inputMethodHints: Qt.ImhNoPredictiveText
                rightPadding: clearBtn.visible ? clearBtn.width + 4 : 0

                Label {
                    id: clearBtn
                    visible: searchField.text.length > 0
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 6 }
                    text: "✕"
                    font.pixelSize: 18
                    color: palette.text
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -6
                        onClicked: searchField.clear()
                    }
                }
            }

            ToolButton {
                icon.name: "line.3.horizontal.decrease.circle"
                onClicked: filterSheet.openFor("people", qsTr("People"))
                Rectangle {
                    visible: { AppController.filterRev; return AppController.hasActiveColumnFilters(AppController.peopleModel) }
                    width: 8; height: 8; radius: 4; color: palette.highlight
                    anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 6 }
                }
            }

            ToolButton {
                icon.name: "arrow.up.arrow.down"
                onClicked: sortSheet.openFor("people", qsTr("People"))
                Rectangle {
                    visible: { AppController.sortRev; return (AppController.activeSort(AppController.peopleModel).field || "") !== "" }
                    width: 8; height: 8; radius: 4; color: palette.highlight
                    anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 6 }
                }
            }

            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addPerson()
                    if (newRow < 0) return
                    var d = AppController.getPersonData(newRow)
                    root.stackView.push(Qt.resolvedUrl("PersonDetailPage.qml"), {
                        personRow:          newRow,
                        personId:           (d.id           || "").toString(),
                        isNewRecord:        true,
                        initialName:        (d.name         || "").toString(),
                        initialEmail:       (d.email        || "").toString(),
                        initialOfficePhone: (d.office_phone || "").toString(),
                        initialCellPhone:   (d.cell_phone   || "").toString(),
                        initialClientId:    (d.client_id    || "").toString(),
                        initialRole:        (d.role         || "").toString()
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.peopleModel
        clip: true
        reuseItems: true

        delegate: ItemDelegate {
            id: delegateRoot
            required property int index
            required property var model
            width: listView.width

            // Long press → Quick Filter. The delegate's own pressAndHold, not a
            // TapHandler: only the button's hold timer (armed solely by
            // connecting to this signal) suppresses the clicked() that would
            // otherwise navigate to this row when you let go — see
            // AllItemsPage.qml.
            onPressAndHold: qfDialog.openWith(AppController.peopleModel, root._quickFiltersForRow(delegateRoot.model))

            contentItem: RowLayout {
                spacing: 12

                Rectangle {
                    width: 38; height: 38
                    radius: 19
                    color: Theme.navyMid
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        anchors.centerIn: parent
                        text: (delegateRoot.model.name || "?").charAt(0).toUpperCase()
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                    }
                }

                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true

                    Label {
                        text: delegateRoot.model.name || ""
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: {
                            var email = delegateRoot.model.email || ""
                            var role  = delegateRoot.model.role  || ""
                            if (email && role) return email + "  ·  " + role
                            return email || role
                        }
                        font.pixelSize: 12
                        color: Theme.mutedText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    spacing: 0
                    Layout.alignment: Qt.AlignVCenter

                    ToolButton {
                        visible: (delegateRoot.model.cell_phone || "").length > 0
                        icon.name: "iphone"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("tel:" + (delegateRoot.model.cell_phone || "").replace(/[^\d+]/g, ""))
                    }

                    ToolButton {
                        visible: (delegateRoot.model.office_phone || "").length > 0
                        icon.name: "phone.fill"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("tel:" + (delegateRoot.model.office_phone || "").replace(/[^\d+]/g, ""))
                    }

                    ToolButton {
                        visible: (delegateRoot.model.email || "").length > 0
                        icon.name: "envelope"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("mailto:" + (delegateRoot.model.email || ""))
                    }
                }
            }
            onClicked: {
                root.stackView.push(Qt.resolvedUrl("PersonDetailPage.qml"), {
                    personRow:         delegateRoot.index,
                    personId:          delegateRoot.model.id           || "",
                    initialName:       delegateRoot.model.name         || "",
                    initialEmail:      delegateRoot.model.email        || "",
                    initialOfficePhone:delegateRoot.model.office_phone || "",
                    initialCellPhone:  delegateRoot.model.cell_phone   || "",
                    initialClientId:   delegateRoot.model.client_id    || "",
                    initialRole:       delegateRoot.model.role         || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Column {
        anchors.centerIn: parent
        visible: listView.count === 0
        spacing: 10

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "\uD83D\uDC64"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No People")
            font.pixelSize: 17
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add a contact.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: Theme.mutedText
        }
    }
}

// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// StatusItemsPage — mirrors the Status tab in the desktop project details view.
// Columns from statusreportitemsmodel.cpp:
//   0=id, 1=project_id, 2=task_category, 3=task_description

Page {
    id: root
    title: {
        var base = qsTr("Status Items")
        if (root.projectId === "") return base
        return base + " — " + AppController.projectNumberForId(root.projectId)
                    + " " + AppController.projectNameForId(root.projectId).substring(0, 20)
    }

    property string projectId:    ""
    property string projectTitle: ""

    StackView.onActivated: AppController.setProjectFilter(root.projectId)

    FilterSheet { id: filterSheet }
    SortSheet   { id: sortSheet }

    header: ToolBar {
        // Glass search capsule, matching the floating tab bar. The bar
        // itself takes the page color so no band shows above the list.
        background: Rectangle { color: Theme.pageBackground }
        z: 1   // let the capsule's shadow fall over the list
        implicitHeight: 60

        GlassBar {
            anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 6; bottomMargin: 6 }
            contentPadding: 0

            RowLayout {
                anchors { fill: parent; leftMargin: 4; rightMargin: 2 }
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search status items…")
                    onTextChanged: AppController.setQuickSearch(AppController.statusReportItemsModel, text)
                    inputMethodHints: Qt.ImhNoPredictiveText
                    leftPadding: 10
                    rightPadding: clearBtn.visible ? clearBtn.width + 12 : 10
                    implicitHeight: 36
                    background: Item {}

                    Label {
                        id: clearBtn
                        visible: searchField.text.length > 0
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 10 }
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
                    icon.color: filterBadge.iconColor
                    onClicked: filterSheet.openFor("statusreport", qsTr("Status Items"))
                    ActiveIndicator {
                        id: filterBadge
                        active: AppController.filterRev >= 0 && AppController.hasActiveColumnFilters(AppController.statusReportItemsModel)
                    }
                }
                ToolButton {
                    icon.name: "arrow.up.arrow.down"
                    icon.color: sortBadge.iconColor
                    onClicked: sortSheet.openFor("statusreport", qsTr("Status Items"))
                    ActiveIndicator {
                        id: sortBadge
                        active: AppController.sortRev >= 0 && (AppController.activeSort(AppController.statusReportItemsModel).field || "") !== ""
                    }
                }
                ToolButton {
                    icon.name: "plus"
                    onClicked: {
                        var newRow = AppController.addStatusItem(root.projectId)
                        if (newRow < 0) return
                        var d = AppController.getStatusItemData(newRow)
                        root.StackView.view.push(Qt.resolvedUrl("StatusItemDetailPage.qml"), {
                            itemRow:            newRow,
                            itemId:             (d.id                || "").toString(),
                            isNewRecord:        true,
                            initialCategory:    (d.task_category    || "").toString(),
                            initialDescription: (d.task_description || "").toString()
                        })
                    }
                }
            }
        }
    }

    function categoryColor(cat) {
        switch (cat) {
            case "In Progress": return "#e07000"
            case "Completed":   return Theme.accentGreenDark
            case "Starting":    return palette.link
            default:            return Theme.mutedText
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.statusReportItemsModel
        clip: true

        delegate: ItemDelegate {
            id: delegateRoot
            required property int index
            required property var model
            width: listView.width

            contentItem: ColumnLayout {
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: delegateRoot.model.task_category || ""
                        font.bold: true
                        font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
                        color: root.categoryColor(delegateRoot.model.task_category || "")
                        Layout.fillWidth: true
                    }
                }

                Label {
                    visible: (delegateRoot.model.task_description || "") !== ""
                    text: delegateRoot.model.task_description || ""
                    font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            onClicked: {
                root.StackView.view.push(Qt.resolvedUrl("StatusItemDetailPage.qml"), {
                    itemRow:             delegateRoot.index,
                    itemId:              delegateRoot.model.id                 || "",
                    initialCategory:     delegateRoot.model.task_category      || "",
                    initialDescription:  delegateRoot.model.task_description   || ""
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
            text: "\uD83D\uDCCA"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No Status Items")
            font.pixelSize: Theme.title2; font.letterSpacing: Theme.tracking(Theme.title2)
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add a status item.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.subheadline; font.letterSpacing: Theme.tracking(Theme.subheadline)
            color: Theme.mutedText
        }
    }
}

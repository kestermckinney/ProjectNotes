// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    // Space below the last row for Main.qml's floating tab bar.
    property real listBottomInset: 0

    id: root
    title: qsTr("Companies")

    property StackView stackView: null

    FilterSheet { id: filterSheet }
    SortSheet   { id: sortSheet }

    // ── Search bar ─────────────────────────────────────────────────────────
    // A glass capsule floating over the top of the list. The rows scroll
    // beneath it and show through frosted, as they do under the tab bar.
    NavButton {
        id: navButton
        atRoot: true
        anchors { left: parent.left; leftMargin: 12; verticalCenter: searchBar.verticalCenter }
    }

    GlassBar {
        id: searchBar
        z: 2
        anchors { top: parent.top; left: navButton.right; right: parent.right; topMargin: 6; leftMargin: 8; rightMargin: 12 }
        height: 48
        contentPadding: 0
        blurSource: listView
        blurRect: Qt.rect(searchBar.x - listView.x, searchBar.y - listView.y,
                          searchBar.width, searchBar.height)

        RowLayout {
            anchors { fill: parent; leftMargin: 4; rightMargin: 2 }

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search companies…")
                onTextChanged: AppController.setQuickSearch(AppController.companiesModel, text)
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

            BarButton {
                icon.name: "line.3.horizontal.decrease.circle"
                icon.color: filterBadge.iconColor
                onClicked: filterSheet.openFor("companies", qsTr("Companies"))
                ActiveIndicator {
                    id: filterBadge
                    active: AppController.filterRev >= 0 && AppController.hasActiveColumnFilters(AppController.companiesModel)
                }
            }

            BarButton {
                icon.name: "arrow.up.arrow.down"
                icon.color: sortBadge.iconColor
                onClicked: sortSheet.openFor("companies", qsTr("Companies"))
                ActiveIndicator {
                    id: sortBadge
                    active: AppController.sortRev >= 0 && (AppController.activeSort(AppController.companiesModel).field || "") !== ""
                }
            }

            BarButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addCompany()
                    if (newRow < 0) return
                    var d = AppController.getCompanyData(newRow)
                    root.stackView.push(Qt.resolvedUrl("CompanyDetailPage.qml"), {
                        companyRow:         newRow,
                        companyId:          (d.id           || "").toString(),
                        isNewRecord:       true,
                        initialCompanyName: (d.client_name || "").toString()
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        topMargin: searchBar.height + 12   // first row clear of the search capsule
        anchors.fill: parent
        bottomMargin: listBottomInset
        model: AppController.companiesModel
        clip: true
        reuseItems: true

        delegate: ItemDelegate {
            id: delegateRoot
            required property int index
            required property var model
            width: listView.width
            contentItem: RowLayout {
                spacing: 12

                Rectangle {
                    width: 38; height: 38
                    radius: 10
                    color: "#5856D6"
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        anchors.centerIn: parent
                        text: (delegateRoot.model.client_name || "?").charAt(0).toUpperCase()
                        font.pixelSize: Theme.callout; font.letterSpacing: Theme.tracking(Theme.callout)
                        font.bold: true
                        color: "white"
                    }
                }

                Label {
                    text: delegateRoot.model.client_name || ""
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
            }
            onClicked: {
                root.stackView.push(Qt.resolvedUrl("CompanyDetailPage.qml"), {
                    companyRow:         delegateRoot.index,
                    companyId:          delegateRoot.model.id           || "",
                    initialCompanyName: delegateRoot.model.client_name || ""
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
            text: "🏢"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No Companies")
            font.pixelSize: Theme.title2; font.letterSpacing: Theme.tracking(Theme.title2)
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add a company.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.subheadline; font.letterSpacing: Theme.tracking(Theme.subheadline)
            color: Theme.mutedText
        }
    }
}

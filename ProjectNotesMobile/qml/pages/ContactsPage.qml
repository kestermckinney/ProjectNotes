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
    title: qsTr("Contacts")

    property StackView stackView: null

    // Quick Filter entry for a long-pressed contact row: pre-filled from that
    // row's own company — client_id is a genuine direct column on the contacts
    // table (contactsmodel.cpp), not derived.
    function _quickFiltersForRow(m) {
        var qf = []
        var companyId = (m.client_id || "").toString()
        if (companyId !== "")
            qf.push({ label: qsTr("This Company"), field: "client_id", values: [companyId] })
        return qf
    }

    FilterSheet     { id: filterSheet }
    SortSheet       { id: sortSheet }
    QuickFilterDialog { id: qfDialog }
    ContactCardDialog {
        id: cardDialog
        property var _quickFilters: []
        onQuickFilterRequested: qfDialog.openWith(AppController.contactsModel, _quickFilters)
    }

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
        anchors { top: parent.top; left: navButton.right; right: parent.right; topMargin: Theme.topInset + 6; leftMargin: 8; rightMargin: 12 }
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
                placeholderText: qsTr("Search contacts…")
                onTextChanged: AppController.setQuickSearch(AppController.contactsModel, text)
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
                onClicked: filterSheet.openFor("contacts", qsTr("Contacts"))
                ActiveIndicator {
                    id: filterBadge
                    active: AppController.filterRev >= 0 && AppController.hasActiveColumnFilters(AppController.contactsModel)
                }
            }

            BarButton {
                icon.name: "arrow.up.arrow.down"
                icon.color: sortBadge.iconColor
                onClicked: sortSheet.openFor("contacts", qsTr("Contacts"))
                ActiveIndicator {
                    id: sortBadge
                    active: AppController.sortRev >= 0 && (AppController.activeSort(AppController.contactsModel).field || "") !== ""
                }
            }

            BarButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addContact()
                    if (newRow < 0) return
                    var d = AppController.getContactData(newRow)
                    root.stackView.push(Qt.resolvedUrl("ContactDetailPage.qml"), {
                        contactRow:         newRow,
                        contactId:          (d.id           || "").toString(),
                        isNewRecord:        true,
                        initialName:        (d.name         || "").toString(),
                        initialEmail:       (d.email        || "").toString(),
                        initialOfficePhone: (d.office_phone || "").toString(),
                        initialCellPhone:   (d.cell_phone   || "").toString(),
                        initialCompanyId:   (d.client_id    || "").toString(),
                        initialRole:        (d.role         || "").toString()
                    })
                }
            }
        }
    }

    ListView {
        id: listView
        topMargin: searchBar.y + searchBar.height + 6   // first row clear of the title bar and search capsule
        // The lists exist before the title bar is measured; start them at the top
        // once the margin settles rather than with the first row underneath it.
        onTopMarginChanged: contentY = -topMargin
        anchors.fill: parent
        bottomMargin: listBottomInset
        model: AppController.contactsModel
        clip: true
        reuseItems: true

        delegate: ItemDelegate {
            id: delegateRoot
            required property int index
            required property var model
            width: listView.width

            // Long press → contact card actions, plus Quick Filter when the row
            // has a company. The delegate's own pressAndHold, not a
            // TapHandler: only the button's hold timer (armed solely by
            // connecting to this signal) suppresses the clicked() that would
            // otherwise navigate to this row when you let go — see
            // AllItemsPage.qml.
            onPressAndHold: {
                cardDialog._quickFilters = root._quickFiltersForRow(delegateRoot.model)
                cardDialog.openFor((delegateRoot.model.id || "").toString(),
                                   (delegateRoot.model.name || "").toString(),
                                   cardDialog._quickFilters.length > 0)
            }

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
                        font.pixelSize: Theme.callout; font.letterSpacing: Theme.tracking(Theme.callout)
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
                        font.pixelSize: Theme.caption1; font.letterSpacing: Theme.tracking(Theme.caption1)
                        color: Theme.mutedText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    spacing: 0
                    Layout.alignment: Qt.AlignVCenter

                    BarButton {
                        visible: (delegateRoot.model.cell_phone || "").length > 0
                        icon.name: "iphone"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("tel:" + (delegateRoot.model.cell_phone || "").replace(/[^\d+]/g, ""))
                    }

                    BarButton {
                        visible: (delegateRoot.model.office_phone || "").length > 0
                        icon.name: "phone.fill"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("tel:" + (delegateRoot.model.office_phone || "").replace(/[^\d+]/g, ""))
                    }

                    BarButton {
                        visible: (delegateRoot.model.email || "").length > 0
                        icon.name: "envelope"
                        implicitWidth: 44; implicitHeight: 44
                        onClicked: Qt.openUrlExternally("mailto:" + (delegateRoot.model.email || ""))
                    }
                }
            }
            onClicked: {
                root.stackView.push(Qt.resolvedUrl("ContactDetailPage.qml"), {
                    contactRow:        delegateRoot.index,
                    contactId:         delegateRoot.model.id           || "",
                    initialName:       delegateRoot.model.name         || "",
                    initialEmail:      delegateRoot.model.email        || "",
                    initialOfficePhone:delegateRoot.model.office_phone || "",
                    initialCellPhone:  delegateRoot.model.cell_phone   || "",
                    initialCompanyId:  delegateRoot.model.client_id    || "",
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
            text: "👤"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No Contacts")
            font.pixelSize: Theme.title2; font.letterSpacing: Theme.tracking(Theme.title2)
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add a contact.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.subheadline; font.letterSpacing: Theme.tracking(Theme.subheadline)
            color: Theme.mutedText
        }
    }
}

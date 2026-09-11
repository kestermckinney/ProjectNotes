// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// ProjectsListPage — master list of projects.
// Tapping a project will push ProjectDetailsPage (to be implemented).
// Columns shown on mobile: project_number, project_name, status (3 of the many desktop columns).
// Column indices are based on ProjectsListModel's SELECT order — see projectslistmodel.cpp.

Page {
    id: root
    title: qsTr("Projects")

    property StackView stackView: null

    // Quick Filter entries for a long-pressed project row: pre-filled from
    // that row's own client/contact (omitted when the row has none set),
    // plus the two always-available overdue toggles — status_overdue/
    // invoicing_overdue are computed "Yes"/"No" columns (see projectsmodel.cpp).
    function _quickFiltersForRow(m) {
        var qf = []
        var clientId = (m.client_id || "").toString()
        if (clientId !== "")
            qf.push({ label: qsTr("This Client"), field: "client_id", values: [clientId] })
        var contactId = (m.primary_contact || "").toString()
        if (contactId !== "")
            qf.push({ label: qsTr("This Contact"), field: "primary_contact", values: [contactId] })
        qf.push({ label: qsTr("Status Overdue"),    field: "status_overdue",    values: ["Yes"] })
        qf.push({ label: qsTr("Invoicing Overdue"), field: "invoicing_overdue", values: ["Yes"] })
        return qf
    }

    FilterSheet     { id: filterSheet }
    SortSheet       { id: sortSheet }
    QuickFilterDialog { id: qfDialog }

    // ── Toolbar: filter toggle + search ──────────────────────────────────────
    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search projects…")
                onTextChanged: AppController.setQuickSearch(AppController.projectsListModel, text)
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
                icon.color: filterBadge.iconColor
                onClicked: filterSheet.openFor("projects", qsTr("Projects"))
                ActiveIndicator {
                    id: filterBadge
                    active: AppController.filterRev >= 0 && AppController.hasActiveColumnFilters(AppController.projectsListModel)
                }
            }

            ToolButton {
                icon.name: "arrow.up.arrow.down"
                icon.color: sortBadge.iconColor
                onClicked: sortSheet.openFor("projects", qsTr("Projects"))
                ActiveIndicator {
                    id: sortBadge
                    active: AppController.sortRev >= 0 && (AppController.activeSort(AppController.projectsListModel).field || "") !== ""
                }
            }

            ToolButton {
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addProject()
                    if (newRow < 0) return
                    var d = AppController.getProjectData(newRow)
                    root.stackView.push(Qt.resolvedUrl("ProjectDetailsPage.qml"), {
                        projectRow:               newRow,
                        isNewRecord:              true,
                        // Staged, so there is no id yet — the page writes the row
                        // once it has a number and name (both required). The
                        // number starts at the next free one so naming the
                        // project is all it takes to create it.
                        projectId:                (d.id                   || "").toString(),
                        initialProjectNumber:     ((d.project_number || "").toString() !== "")
                                                      ? (d.project_number || "").toString()
                                                      : AppController.nextProjectNumber(),
                        initialProjectName:       (d.project_name         || "").toString(),
                        initialProjectStatus:     (d.project_status       || "").toString(),
                        initialPrimaryContact:    (d.primary_contact      || "").toString(),
                        initialClientId:          (d.client_id            || "").toString(),
                        initialLastStatusDate:    (d.last_status_date     || "").toString(),
                        initialLastInvoiceDate:   (d.last_invoice_date    || "").toString(),
                        initialInvoicingPeriod:   (d.invoicing_period     || "").toString(),
                        initialStatusReportPeriod:(d.status_report_period || "").toString(),
                        initialBudget:            (d.budget               || "").toString(),
                        initialActual:            (d.actual               || "").toString(),
                        initialBcwp:              (d.bcwp                 || "").toString(),
                        initialBcws:              (d.bcws                 || "").toString(),
                        initialBac:               (d.bac                  || "").toString(),
                        initialPctConsumed:       (d.pct_consumed         || "").toString(),
                        initialEac:               (d.eac                  || "").toString(),
                        initialCv:                (d.cv                   || "").toString(),
                        initialSv:                (d.sv                   || "").toString(),
                        initialPctComplete:       (d.pct_complete         || "").toString(),
                        initialCpi:               (d.cpi                  || "").toString()
                    })
                }
            }

        }
    }

    // ── Project list ─────────────────────────────────────────────────────────
    ListView {
        id: listView
        anchors.fill: parent
        model: AppController.projectsListModel
        clip: true
        reuseItems: true

        delegate: ItemDelegate {
            id: delegateRoot
            required property int index
            required property var model
            width: listView.width

            // Resolve once per delegate; the binding re-evaluates only when the
            // row's client_id changes. Avoids a second linear-scan per row.
            readonly property string _clientName:
                AppController.clientNameForId(delegateRoot.model.client_id || "") || ""

            // Long press → Quick Filter. The delegate's own pressAndHold, not a
            // TapHandler: only the button's hold timer (armed solely by
            // connecting to this signal) suppresses the clicked() that would
            // otherwise navigate to this row when you let go — see
            // AllItemsPage.qml.
            onPressAndHold: qfDialog.openWith(AppController.projectsListModel, root._quickFiltersForRow(delegateRoot.model))

            contentItem: ColumnLayout {
                spacing: 4

                Label {
                    text: {
                        var num  = delegateRoot.model.project_number || ""
                        var name = delegateRoot.model.project_name   || ""
                        return num ? num + "  " + name : name
                    }
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        visible: delegateRoot._clientName !== ""
                        text: delegateRoot._clientName
                        font.pixelSize: 12
                        color: Theme.mutedText
                        elide: Text.ElideRight
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        visible: (delegateRoot.model.project_status || "") !== ""
                        width: 7; height: 7; radius: 4
                        color: {
                            var s = delegateRoot.model.project_status || ""
                            if (s === "Active")                      return Theme.accentGreen
                            if (s === "On Hold")                     return "#ff9500"
                            if (s === "Closed" || s === "Complete")  return "#8e8e93"
                            return Theme.navyMid
                        }
                    }

                    Label {
                        visible: (delegateRoot.model.project_status || "") !== ""
                        text: delegateRoot.model.project_status || ""
                        font.pixelSize: 12
                        color: {
                            var s = delegateRoot.model.project_status || ""
                            if (s === "Active")                      return Theme.accentGreenDark
                            if (s === "On Hold")                     return "#e07000"
                            if (s === "Closed" || s === "Complete")  return Theme.mutedText
                            return Theme.navyMid
                        }
                        elide: Text.ElideRight
                    }
                }
            }

            onClicked: {
                root.stackView.push(Qt.resolvedUrl("ProjectDetailsPage.qml"), {
                    projectRow:               delegateRoot.index,
                    projectId:                delegateRoot.model.id                    || "",
                    initialProjectNumber:     delegateRoot.model.project_number        || "",
                    initialProjectName:       delegateRoot.model.project_name          || "",
                    initialProjectStatus:     delegateRoot.model.project_status        || "",
                    initialPrimaryContact:    delegateRoot.model.primary_contact       || "",
                    initialClientId:          delegateRoot.model.client_id             || "",
                    initialLastStatusDate:    delegateRoot.model.last_status_date      || "",
                    initialLastInvoiceDate:   delegateRoot.model.last_invoice_date     || "",
                    initialInvoicingPeriod:   delegateRoot.model.invoicing_period      || "",
                    initialStatusReportPeriod:delegateRoot.model.status_report_period  || "",
                    initialBudget:            delegateRoot.model.budget                || "",
                    initialActual:            delegateRoot.model.actual                || "",
                    initialBcwp:              delegateRoot.model.bcwp                  || "",
                    initialBcws:              delegateRoot.model.bcws                  || "",
                    initialBac:               delegateRoot.model.bac                   || "",
                    initialPctConsumed:       delegateRoot.model.pct_consumed          || "",
                    initialEac:               delegateRoot.model.eac                   || "",
                    initialCv:                delegateRoot.model.cv                    || "",
                    initialSv:                delegateRoot.model.sv                    || "",
                    initialPctComplete:       delegateRoot.model.pct_complete          || "",
                    initialCpi:               delegateRoot.model.cpi                   || ""
                })
            }
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }

    // ── Empty state ───────────────────────────────────────────────────────────
    Column {
        anchors.centerIn: parent
        visible: listView.count === 0
        spacing: 10

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "\uD83D\uDCC2"
            font.pixelSize: 52
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No Projects")
            font.pixelSize: 17
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Tap + to add one or sync to load your data.")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: Theme.mutedText
        }
    }
    // ── Startup ───────────────────────────────────────────────────────────────
    // Defer DB init so the QML shell renders its first frame before the
    // synchronous SQL work begins — eliminates the black-screen delay.
    // Component.onCompleted: Qt.callLater(function() {
    //     AppController.projectsListModel.refresh()
    // })
}

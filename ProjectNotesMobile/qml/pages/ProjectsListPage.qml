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
                icon.name: "plus"
                onClicked: {
                    var newRow = AppController.addProject()
                    if (newRow < 0) return
                    var d = AppController.getProjectData(newRow)
                    root.stackView.push(Qt.resolvedUrl("ProjectDetailsPage.qml"), {
                        projectRow:               newRow,
                        projectId:                (d.id                   || "").toString(),
                        initialProjectNumber:     (d.project_number       || "").toString(),
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

        delegate: ItemDelegate {
            width: listView.width
            contentItem: ColumnLayout {
                spacing: 4

                Label {
                    text: {
                        var num  = model.project_number || ""
                        var name = model.project_name   || ""
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
                        visible: (AppController.clientNameForId(model.client_id || "") || "") !== ""
                        text: AppController.clientNameForId(model.client_id || "") || ""
                        font.pixelSize: 12
                        color: palette.placeholderText
                        elide: Text.ElideRight
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        visible: (model.project_status || "") !== ""
                        width: 7; height: 7; radius: 4
                        color: {
                            var s = model.project_status || ""
                            if (s === "Active")                      return Theme.accentGreen
                            if (s === "On Hold")                     return "#ff9500"
                            if (s === "Closed" || s === "Complete")  return "#8e8e93"
                            return Theme.navyMid
                        }
                    }

                    Label {
                        visible: (model.project_status || "") !== ""
                        text: model.project_status || ""
                        font.pixelSize: 12
                        color: {
                            var s = model.project_status || ""
                            if (s === "Active")                      return Theme.accentGreenDark
                            if (s === "On Hold")                     return "#e07000"
                            if (s === "Closed" || s === "Complete")  return palette.placeholderText
                            return Theme.navyMid
                        }
                        elide: Text.ElideRight
                    }
                }
            }

            onClicked: {
                root.stackView.push(Qt.resolvedUrl("ProjectDetailsPage.qml"), {
                    projectRow:               index,
                    projectId:                model.id                    || "",
                    initialProjectNumber:     model.project_number        || "",
                    initialProjectName:       model.project_name          || "",
                    initialProjectStatus:     model.project_status        || "",
                    initialPrimaryContact:    model.primary_contact       || "",
                    initialClientId:          model.client_id             || "",
                    initialLastStatusDate:    model.last_status_date      || "",
                    initialLastInvoiceDate:   model.last_invoice_date     || "",
                    initialInvoicingPeriod:   model.invoicing_period      || "",
                    initialStatusReportPeriod:model.status_report_period  || "",
                    initialBudget:            model.budget                || "",
                    initialActual:            model.actual                || "",
                    initialBcwp:              model.bcwp                  || "",
                    initialBcws:              model.bcws                  || "",
                    initialBac:               model.bac                   || "",
                    initialPctConsumed:       model.pct_consumed          || "",
                    initialEac:               model.eac                   || "",
                    initialCv:                model.cv                    || "",
                    initialSv:                model.sv                    || "",
                    initialPctComplete:       model.pct_complete          || "",
                    initialCpi:               model.cpi                   || ""
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
            color: palette.placeholderText
        }
    }
}

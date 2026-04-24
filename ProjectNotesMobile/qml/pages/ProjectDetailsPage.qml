// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

// ProjectDetailsPage — mirrors the desktop Status tab fields.
// Column indices from projectsmodel.cpp / projectslistmodel.cpp:
//   0=id, 1=project_number, 2=project_name, 3=last_status_date,
//   4=last_invoice_date, 5=primary_contact, 6=budget, 7=actual,
//   8=bcwp, 9=bcws, 10=bac, 11=invoicing_period, 12=status_report_period,
//   13=client_id, 14=project_status

Page {
    id: root
    title: qsTr("Project")

    property int    projectRow:                -1
    property string projectId:                ""
    property string initialProjectNumber:     ""
    property string initialProjectName:       ""
    property string initialProjectStatus:     ""
    property string initialPrimaryContact:    ""
    property string initialClientId:          ""
    property string initialLastStatusDate:    ""
    property string initialLastInvoiceDate:   ""
    property string initialInvoicingPeriod:   ""
    property string initialStatusReportPeriod: ""
    property string initialBudget:            ""
    property string initialActual:            ""
    property string initialBcwp:              ""
    property string initialBcws:              ""
    property string initialBac:               ""
    property string initialPctConsumed:       ""
    property string initialEac:              ""
    property string initialCv:               ""
    property string initialSv:               ""
    property string initialPctComplete:       ""
    property string initialCpi:              ""
    property bool   _skipSave:                false

    function _saveNow() {
        statusDateField.commitPending()
        invoiceDateField.commitPending()
        var primaryContactId = (primaryContactCombo.currentIndex >= 0)
            ? AppController.teamMemberPersonIdAtRow(primaryContactCombo.currentIndex) : ""
        var clientId = (clientCombo.currentIndex >= 0)
            ? AppController.clientIdAtRow(clientCombo.currentIndex) : ""
        var status = (statusCombo.currentIndex >= 0)
            ? statusCombo.model[statusCombo.currentIndex] : ""
        var invPeriod = (invoicingCombo.currentIndex >= 0)
            ? invoicingCombo.model[invoicingCombo.currentIndex] : ""
        var srPeriod = (statusReportCombo.currentIndex >= 0)
            ? statusReportCombo.model[statusReportCombo.currentIndex] : ""
        AppController.saveProject(root.projectRow, numberField.text, nameField.text,
                                  status, primaryContactId, clientId, statusDateField.text,
                                  invoiceDateField.text, invPeriod, srPeriod)
    }

    Component.onCompleted: {
        if (projectId !== "")
            AppController.setProjectFilter(projectId)
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

    // ── Toolbar: email + copy + delete ───────────────────────────────────────
    header: ToolBar {
        RowLayout {
            anchors { left: parent.left; right: parent.right; margins: 8 }
            height: parent.height
            Item { Layout.fillWidth: true }

            ToolButton {
                icon.name: "envelope"
                onClicked: {
                    var emails  = AppController.teamMemberEmailList()
                    var subject = numberField.text + " " + nameField.text + " -"
                    if (emails !== "")
                        Qt.openUrlExternally("mailto:" + emails + "?subject=" + encodeURIComponent(subject))
                }
            }

            ToolButton {
                icon.name: "doc.on.doc"
                onClicked: {
                    root._saveNow()
                    root._skipSave = true
                    var newRow = AppController.copyProject(root.projectRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getProjectData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("ProjectDetailsPage.qml"), {
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

            ToolButton {
                icon.name: "trash"
                onClicked: {
                    root._skipSave = true
                    AppController.deleteProject(root.projectRow)
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

            // ── Identity ──────────────────────────────────────────────────────

            SectionHeader { text: qsTr("Project Number") }
            FieldRow {
                TextField {
                    id: numberField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialProjectNumber
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Project Name") }
            FieldRow {
                TextField {
                    id: nameField
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: root.initialProjectName
                    horizontalAlignment: TextInput.AlignLeft
                    inputMethodHints: Qt.ImhNoPredictiveText
                    background: Item {}
                }
            }

            SectionHeader { text: qsTr("Status") }
            FieldRow {
                ComboBox {
                    id: statusCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.projectStatusOptions()
                    Component.onCompleted: {
                        var idx = model.indexOf(root.initialProjectStatus)
                        currentIndex = (idx >= 0) ? idx : 0
                    }
                }
            }

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

            SectionHeader { text: qsTr("Primary Contact") }
            FieldRow {
                ComboBox {
                    id: primaryContactCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.projectTeamMembersModel
                    textRole: "name"
                    Component.onCompleted: {
                        var row = AppController.teamMemberRowForPersonId(root.initialPrimaryContact)
                        currentIndex = (row >= 0) ? row : -1
                    }
                }
            }

            // ── Dates ─────────────────────────────────────────────────────────

            SectionHeader { text: qsTr("Last Status Date") }
            DateFieldRow { id: statusDateField;  text: root.initialLastStatusDate }

            SectionHeader { text: qsTr("Last Invoice Date") }
            DateFieldRow { id: invoiceDateField; text: root.initialLastInvoiceDate }

            // ── Periods ───────────────────────────────────────────────────────

            SectionHeader { text: qsTr("Invoice Period") }
            FieldRow {
                ComboBox {
                    id: invoicingCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.invoicingPeriodOptions()
                    Component.onCompleted: {
                        var idx = model.indexOf(root.initialInvoicingPeriod)
                        currentIndex = (idx >= 0) ? idx : -1
                    }
                }
            }

            SectionHeader { text: qsTr("Status Report Period") }
            FieldRow {
                ComboBox {
                    id: statusReportCombo
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 8; rightMargin: 8 }
                    model: AppController.statusReportPeriodOptions()
                    Component.onCompleted: {
                        var idx = model.indexOf(root.initialStatusReportPeriod)
                        currentIndex = (idx >= 0) ? idx : -1
                    }
                }
            }

            // ── Earned Value ──────────────────────────────────────────────────

            SectionHeader { text: qsTr("Budget") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialBudget); return (!isNaN(v) && root.initialBudget !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialBudget)) && root.initialBudget !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("Actual") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialActual); return (!isNaN(v) && root.initialActual !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialActual)) && root.initialActual !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("BCWP") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialBcwp); return (!isNaN(v) && root.initialBcwp !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialBcwp)) && root.initialBcwp !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("BCWS") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialBcws); return (!isNaN(v) && root.initialBcws !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialBcws)) && root.initialBcws !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("BAC") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialBac); return (!isNaN(v) && root.initialBac !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialBac)) && root.initialBac !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("% Consumed") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialPctConsumed); return (!isNaN(v) && root.initialPctConsumed !== "") ? v.toLocaleString(Qt.locale(), "f", 2) + "%" : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialPctConsumed)) && root.initialPctConsumed !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("EAC") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialEac); return (!isNaN(v) && root.initialEac !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialEac)) && root.initialEac !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("CV") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialCv); return (!isNaN(v) && root.initialCv !== "") ? v.toLocaleString(Qt.locale(), "f", 2) + "%" : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialCv)) && root.initialCv !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("SV") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialSv); return (!isNaN(v) && root.initialSv !== "") ? v.toLocaleString(Qt.locale(), "f", 2) + "%" : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialSv)) && root.initialSv !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("% Complete") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialPctComplete); return (!isNaN(v) && root.initialPctComplete !== "") ? v.toLocaleString(Qt.locale(), "f", 2) + "%" : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialPctComplete)) && root.initialPctComplete !== "") ? palette.text : palette.placeholderText
                }
            }

            SectionHeader { text: qsTr("CPI") }
            FieldRow {
                Label {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
                    text: { var v = parseFloat(root.initialCpi); return (!isNaN(v) && root.initialCpi !== "") ? v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                    color: (!isNaN(parseFloat(root.initialCpi)) && root.initialCpi !== "") ? palette.text : palette.placeholderText
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    // ── Bottom navigation to project sub-pages ───────────────────────────────
    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 0

            Repeater {
                model: [
                    { label: qsTr("Status"),  icon: "chart.bar.doc.horizontal", page: "StatusItemsPage.qml"    },
                    { label: qsTr("Team"),    icon: "person.2",                  page: "TeamMembersPage.qml"    },
                    { label: qsTr("Items"),   icon: "exclamationmark.triangle",  page: "ProjectTrackerPage.qml" },
                    { label: qsTr("Files"),   icon: "folder",                    page: "ProjectLocationsPage.qml"},
                    { label: qsTr("Notes"),   icon: "doc.text",                  page: "ProjectNotesPage.qml"   }
                ]

                ToolButton {
                    Layout.fillWidth: true
                    icon.name: modelData.icon
                    ToolTip.text: modelData.label
                    ToolTip.visible: hovered || pressed
                    display: AbstractButton.IconOnly
                    onClicked: root.StackView.view.push(
                        Qt.resolvedUrl(modelData.page),
                        { projectId: root.projectId,
                          projectTitle: numberField.text + " " + nameField.text }
                    )
                }
            }
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
            color: palette.placeholderText
            opacity: 0.3
        }
    }
}

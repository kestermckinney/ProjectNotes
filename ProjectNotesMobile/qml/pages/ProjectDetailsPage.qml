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
    property string initialCompanyId:         ""
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
    property bool   isNewRecord:              false

    // Stable {id,name} snapshot backing primaryContactCombo — NOT the live
    // AppController.projectTeamMembersModel proxy, which the Team tab's Sort
    // feature can reorder/reset out from under a ComboBox bound directly to
    // it (see AppController::teamMemberList doc comment).
    property var _contacts: []
    function _contactNames() { return root._contacts.map(function(p){ return p.name }) }
    function _contactIndexForId(id) {
        for (var i = 0; i < root._contacts.length; i++)
            if (root._contacts[i].id === id) return i
        return -1
    }
    // Same rationale, backing companyCombo — companiesModel is also Sort-able.
    property var _companies: []
    function _companyNames() { return root._companies.map(function(c){ return c.name }) }
    function _companyIndexForId(id) {
        for (var i = 0; i < root._companies.length; i++)
            if (root._companies[i].id === id) return i
        return -1
    }

    function _isBlankNew() { return isNewRecord && nameField.text.trim() === "" }
    // Covers both states a new project can be in: still staged in the model cache
    // (no id — deleteProject just drops the cache row) or already written.
    function _discardNew()  {
        var row = AppController.rowForId(AppController.projectsListModel, root.projectId)
        if (row < 0) return
        AppController.deleteProject(row)
    }

    // Re-resolve projectRow from the stable projectId before every write —
    // Sort/refresh elsewhere in the app can reorder or reset the shared
    // projectsListModel proxy while this page is open. A project that hasn't been
    // created yet has no id; rowForId() resolves the staged row by that empty key.
    function _saveNow() {
        // A staged project needs the two fields the schema requires of it before
        // the save below can create it. Without a name there is nothing to create
        // yet; without a number, fall back to the next free one rather than
        // leaving a named project uncreatable.
        if (root.projectId === "") {
            if (nameField.text.trim() === "") return true
            if (numberField.text.trim() === "")
                numberField.text = AppController.nextProjectNumber()
        }

        var row = AppController.rowForId(AppController.projectsListModel, root.projectId)
        if (row < 0) return false   // record no longer exists
        root.projectRow = row
        statusDateField.commitPending()
        invoiceDateField.commitPending()
        var pi = primaryContactCombo.optionIndex
        var primaryContactId = (pi >= 0 && pi < root._contacts.length) ? root._contacts[pi].id : ""
        var ci = companyCombo.optionIndex
        var companyId = (ci >= 0 && ci < root._companies.length) ? root._companies[ci].id : ""
        var status    = statusCombo.selection
        var invPeriod = invoicingCombo.selection
        var srPeriod  = statusReportCombo.selection
        var wasStaged = root.projectId === ""
        var ok = AppController.saveProject(root.projectRow, numberField.text, nameField.text,
                                          status, primaryContactId, companyId, statusDateField.text,
                                          invoiceDateField.text, invPeriod, srPeriod)
        if (ok && wasStaged) root._adoptSavedRecord()
        return ok
    }

    // The staged row has just been written: take on its new id (the insert can
    // move it within the sorted list, so re-resolve the row too) and point the
    // project-scoped models at it, which is what the sub-pages below read.
    function _adoptSavedRecord() {
        var id = AppController.lastCreatedProjectId()
        if (id === "") return
        root.projectId   = id
        root.isNewRecord = false
        var row = AppController.rowForId(AppController.projectsListModel, id)
        if (row >= 0) root.projectRow = row
        AppController.setProjectFilter(id)
        root._reloadData()   // picks up the default project manager just added
    }

    function _reloadData() {
        var d = AppController.getProjectData(root.projectRow)
        numberField.text = (d.project_number || "").toString()
        nameField.text   = (d.project_name   || "").toString()
        statusCombo.selectText((d.project_status || "").toString(), 0)
        root._companies = AppController.companyList()
        companyCombo.selectOption(root._companyIndexForId((d.client_id || "").toString()))
        var contactId = (d.primary_contact || "").toString()
        root._contacts = AppController.teamMemberList(root.projectId, [contactId])
        primaryContactCombo.selectOption(root._contactIndexForId(contactId))
        statusDateField.text  = (d.last_status_date     || "").toString()
        invoiceDateField.text = (d.last_invoice_date    || "").toString()
        invoicingCombo.selectText((d.invoicing_period || "").toString())
        statusReportCombo.selectText((d.status_report_period || "").toString())
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
    // ── Actions ──────────────────────────────────────────────────────────────
    // Glass pill floating at the top right, over the form as it scrolls.
    NavButton {
        id: navButton
        anchors { left: parent.left; leftMargin: 12; verticalCenter: topBar.verticalCenter }
    }

    GlassBar {
        id: topBar
        z: 2
        anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 12 }
        height: 44
        width: topBarRow.implicitWidth + 2 * contentPadding
        contentPadding: 6
        blurSource: scrollView
        blurRect: Qt.rect(topBar.x - scrollView.x, topBar.y - scrollView.y,
                          topBar.width, topBar.height)

        RowLayout {
            id: topBarRow
            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
            spacing: 0
            BarButton {
                icon.name: "envelope"
                onClicked: {
                    var emails  = AppController.teamMemberEmailList()
                    var subject = numberField.text + " " + nameField.text + " -"
                    if (emails !== "")
                        Qt.openUrlExternally("mailto:" + emails + "?subject=" + encodeURIComponent(subject))
                }
            }

            BarButton {
                icon.name: "doc.on.doc"
                // Nothing to duplicate until the project exists.
                enabled: root.projectId !== ""
                onClicked: {
                    if (!root._saveNow()) return
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
                        initialCompanyId:         (d.client_id            || "").toString(),
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

            BarButton {
                icon.name: "trash"
                // A project that was never written is discarded by leaving the
                // page, not deleted (see _discardNew).
                enabled: root.projectId !== ""
                onClicked: {
                    var row = AppController.rowForId(AppController.projectsListModel, root.projectId)
                    if (row >= 0 && AppController.deleteProject(row)) {
                        root._skipSave = true
                        root.StackView.view.pop()
                    }
                }
            }
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0
            // Room for the floating action pill above the first field.
            Item { Layout.preferredHeight: topBar.height + 6 }


            // ── Identity ──────────────────────────────────────────────────────

            SectionHeader { text: qsTr("Project Number") }
            FieldRow {
                FormField {
                    id: numberField
                    text: root.initialProjectNumber
                    inputMethodHints: Qt.ImhNoPredictiveText
                }
            }

            SectionHeader { text: qsTr("Project Name") }
            FieldRow {
                FormField {
                    id: nameField
                    text: root.initialProjectName
                    inputMethodHints: Qt.ImhNoPredictiveText
                }
            }

            // A project that hasn't been created yet: say so, and say what
            // creates it. The sub-pages in the footer stay disabled until then.
            Label {
                visible: root.projectId === ""
                Layout.fillWidth: true
                Layout.topMargin: 6
                leftPadding: 16
                rightPadding: 16
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.caption1; font.letterSpacing: Theme.tracking(Theme.caption1)
                color: Theme.mutedText
                text: qsTr("New project — give it a name to create it. The pages below become available once it is saved.")
            }

            SectionHeader { text: qsTr("Status") }
            FieldRow {
                FormCombo {
                    id: statusCombo
                    options: AppController.projectStatusOptions()
                    Component.onCompleted: selectText(root.initialProjectStatus, 0)
                }
            }

            SectionHeader { text: qsTr("Company") }
            FieldRow {
                FormCombo {
                    id: companyCombo
                    options: root._companyNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._companies = AppController.companyList()
                        selectOption(root._companyIndexForId(root.initialCompanyId))
                    }
                }
            }

            SectionHeader { text: qsTr("Primary Contact") }
            FieldRow {
                FormCombo {
                    id: primaryContactCombo
                    options: root._contactNames()
                    includeNone: true
                    Component.onCompleted: {
                        root._contacts = AppController.teamMemberList(root.projectId, [root.initialPrimaryContact])
                        selectOption(root._contactIndexForId(root.initialPrimaryContact))
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
                FormCombo {
                    id: invoicingCombo
                    options: AppController.invoicingPeriodOptions()
                    Component.onCompleted: selectText(root.initialInvoicingPeriod)
                }
            }

            SectionHeader { text: qsTr("Status Report Period") }
            FieldRow {
                FormCombo {
                    id: statusReportCombo
                    options: AppController.statusReportPeriodOptions()
                    Component.onCompleted: selectText(root.initialStatusReportPeriod)
                }
            }

            // ── Earned Value ──────────────────────────────────────────────────

            SectionHeader { text: qsTr("Budget") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialBudget); return (!isNaN(v) && root.initialBudget !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("Actual") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialActual); return (!isNaN(v) && root.initialActual !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("BCWP") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialBcwp); return (!isNaN(v) && root.initialBcwp !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("BCWS") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialBcws); return (!isNaN(v) && root.initialBcws !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("BAC") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialBac); return (!isNaN(v) && root.initialBac !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("% Consumed") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialPctConsumed); return (!isNaN(v) && root.initialPctConsumed !== "") ? v.toLocaleString(Qt.locale(), "f", 2) + "%" : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("EAC") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialEac); return (!isNaN(v) && root.initialEac !== "") ? "$" + v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("CV") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialCv); return (!isNaN(v) && root.initialCv !== "") ? v.toLocaleString(Qt.locale(), "f", 2) + "%" : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("SV") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialSv); return (!isNaN(v) && root.initialSv !== "") ? v.toLocaleString(Qt.locale(), "f", 2) + "%" : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("% Complete") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialPctComplete); return (!isNaN(v) && root.initialPctComplete !== "") ? v.toLocaleString(Qt.locale(), "f", 2) + "%" : qsTr("—") }
                }
            }

            SectionHeader { text: qsTr("CPI") }
            FieldRow {
                FormLabel {
                    text: { var v = parseFloat(root.initialCpi); return (!isNaN(v) && root.initialCpi !== "") ? v.toLocaleString(Qt.locale(), "f", 2) : qsTr("—") }
                }
            }

            Item { Layout.preferredHeight: 24 }
            // Room for the floating bottom bar below the last field.
            Item { Layout.preferredHeight: bottomBar.visible ? bottomBar.height + 16 : 0 }
        }
    }

    // ── Bottom navigation to project sub-pages ───────────────────────────────
    // ── Bottom bar ───────────────────────────────────────────────────────────
    // Glass capsule floating over the bottom of the form, like the tab bar.
    GlassBar {
        id: bottomBar
        z: 2
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; bottomMargin: 8; leftMargin: 16; rightMargin: 16 }
        height: 60
        blurSource: scrollView
        blurRect: Qt.rect(bottomBar.x - scrollView.x, bottomBar.y - scrollView.y,
                          bottomBar.width, bottomBar.height)

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Repeater {
                model: [
                    { label: qsTr("Status"),  icon: "chart.bar.doc.horizontal", page: "StatusItemsPage.qml"    },
                    { label: qsTr("Team"),    icon: "person.2",                  page: "TeamMembersPage.qml"    },
                    { label: qsTr("Items"),   icon: "checklist",  page: "ProjectTrackerPage.qml" },
                    { label: qsTr("Files"),   icon: "folder",                    page: "ProjectLocationsPage.qml"},
                    { label: qsTr("Notes"),   icon: "square.and.pencil",         page: "ProjectNotesPage.qml"   }
                ]

                BarButton {
                    Layout.fillWidth: true
                    icon.name: modelData.icon
                    ToolTip.text: modelData.label
                    ToolTip.visible: hovered || pressed
                    display: AbstractButton.IconOnly
                    // Every one of these lists is keyed by the project id, so
                    // none of them exist until the project has been created.
                    enabled: root.projectId !== ""
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
        Layout.topMargin: 10
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
        font.weight: 600
        color: Theme.sectionText
    }
}

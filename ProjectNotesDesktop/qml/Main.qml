// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import ProjectNotesDesktop

ApplicationWindow {
    id: root
    visible: true
    width: 1200
    height: 800
    minimumWidth: 900
    minimumHeight: 560
    title: qsTr("Project Notes")
    color: Theme.bg

    property string currentSection: "projects"
    property string selectedProjectId: ""

    // Breadcrumb subtitle for the current detail page (project / note).
    property string crumbSub: ""

    readonly property var sectionMeta: ({
        "projects": { icon: "description", title: "Projects", newLabel: "New Project", search: true,  add: true  },
        "items":    { icon: "task_alt",    title: "Items",    newLabel: "",            search: true,  add: false },
        "people":   { icon: "group",       title: "People",   newLabel: "New Person",  search: true,  add: true  },
        "clients":  { icon: "apartment",   title: "Clients",  newLabel: "New Client",  search: true,  add: true  },
        "search":   { icon: "search",      title: "Search",   newLabel: "",            search: false, add: false },
        "settings": { icon: "settings",    title: "Settings", newLabel: "",            search: false, add: false }
    })
    readonly property var meta: sectionMeta[currentSection]

    Component.onCompleted: {
        if (DesktopAppController.openOrCreateDatabase())
            FolderManager.reload()
    }

    Connections {
        target: DesktopAppController
        function onErrorOccurred(title, message) {
            errorDialog.title = title
            errorLabel.text = message
            errorDialog.open()
        }
    }

    // ── Navigation ────────────────────────────────────────────────────────────
    function _saveCurrent() {
        var it = contentStack.currentItem
        if (it && typeof it._saveNow === "function")
            it._saveNow()
    }

    function selectSection(section) {
        if (section === root.currentSection && contentStack.depth <= 1)
            return
        _saveCurrent()
        root.currentSection = section
        root.crumbSub = ""
        contentStack.clear()
        switch (section) {
        case "projects": contentStack.push(projectsComponent); break
        case "people":   contentStack.push(peopleComponent); break
        case "clients":  contentStack.push(clientsComponent); break
        case "items":    contentStack.push(itemsComponent); break
        case "search":   contentStack.push(searchComponent); break
        case "settings": contentStack.push(settingsComponent); break
        default:         contentStack.push(stubComponent); break
        }
    }

    // Navigate to a search result based on its datatype.
    function openSearchResult(dataType, dataId, fkId) {
        switch (dataType) {
        case "Project":
            openProject(dataId); break
        case "People":
            openPerson(DesktopAppController.peopleRowForId(dataId), dataId); break
        case "Client":
            openClient(DesktopAppController.clientRowForId(dataId), dataId); break
        case "Item Tracker":
            openItem(dataId); break
        case "Tracker Update":
            openItem(fkId !== "" ? fkId : dataId); break
        default:
            // Notes / attendees / locations / team / status → open the parent project.
            if (fkId !== "") openProject(fkId)
            break
        }
    }

    function openProject(projectId) {
        if (projectId === "") return
        _saveCurrent()
        root.selectedProjectId = projectId
        var r = DesktopAppController.projectRowForId(projectId)
        if (r < 0) return
        root.crumbSub = DesktopAppController.projectNumberForId(projectId)
                        + " " + DesktopAppController.projectNameForId(projectId)
        contentStack.push(projectDetailComponent, { projectRow: r, projectId: projectId })
    }

    function openNote(noteRow, noteId, projectId) {
        _saveCurrent()
        root.crumbSub = qsTr("Note")
        contentStack.push(noteDetailComponent,
                          { noteRow: noteRow, noteId: noteId, projectId: projectId })
    }

    function openPerson(row, personId) {
        _saveCurrent()
        root.crumbSub = DesktopAppController.peopleNameForId(personId)
        contentStack.push(personDetailComponent, { personRow: row, personId: personId })
    }

    function openClient(row, clientId) {
        _saveCurrent()
        root.crumbSub = DesktopAppController.clientNameForId(clientId)
        contentStack.push(clientDetailComponent, { clientRow: row, clientId: clientId })
    }

    function openItem(itemId) {
        if (itemId === "") return
        _saveCurrent()
        root.crumbSub = qsTr("Item")
        contentStack.push(itemDetailComponent, { itemId: itemId })
    }

    // Create a new record in the current section (shared by the TopBar + menu).
    function addForCurrentSection() {
        if (root.currentSection === "projects") {
            var r = DesktopAppController.addProject()
            if (r >= 0) root.openProject(DesktopAppController.projectIdAtRow(r))
        } else if (root.currentSection === "people") {
            var pr = DesktopAppController.addPerson()
            if (pr >= 0) root.openPerson(pr, DesktopAppController.personIdAtRow(pr))
        } else if (root.currentSection === "clients") {
            var cr = DesktopAppController.addClient()
            if (cr >= 0) root.openClient(cr, DesktopAppController.clientIdAtProxyRow(cr))
        }
    }

    // Dispatch a hamburger-menu action.
    function handleMenuAction(action) {
        switch (action) {
        case "new":         addForCurrentSection(); break
        case "search":
        case "find":        selectSection("search"); break
        case "export":
            var it = contentStack.currentItem
            if (it && it.exportTable !== undefined && it.exportTable !== "" && it.exportId !== "")
                exportRecord(it.exportTable, it.exportId)
            break
        case "import":      importDialog.open(); break
        case "preferences":
        case "about":       selectSection("settings"); break
        case "sync":        DesktopAppController.syncNow(); break
        case "filter":      filterDialog.openFor(root.currentSection); break
        case "exit":        Qt.quit(); break
        }
    }

    // ── XML export ────────────────────────────────────────────────────────────
    property string _exportTable: ""
    property string _exportId: ""
    function exportRecord(table, id) {
        if (!table || !id) return
        _saveCurrent()
        root._exportTable = table
        root._exportId = id
        exportDialog.open()
    }

    function goBack() {
        _saveCurrent()
        if (contentStack.depth > 1) {
            contentStack.pop()
            root.crumbSub = contentStack.depth > 1 ? root.crumbSub : ""
            // Recompute crumb for the page we returned to.
            var it = contentStack.currentItem
            root.crumbSub = (it && it.projectId !== undefined && it.noteRow === undefined)
                ? DesktopAppController.projectNumberForId(it.projectId) + " "
                  + DesktopAppController.projectNameForId(it.projectId)
                : ""
        }
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        spacing: 0

        IconRail {
            Layout.fillHeight: true
            currentSection: root.currentSection
            onSectionActivated: (s) => root.selectSection(s)
            onMenuAction: (a) => root.handleMenuAction(a)
        }

        ProjectSidebar {
            id: sidebar
            Layout.fillHeight: true
            visible: root.currentSection === "projects"
            dragLayer: dragOverlay
            selectedProjectId: root.selectedProjectId
            onProjectActivated: (pid) => root.openProject(pid)
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            TopBar {
                Layout.fillWidth: true
                crumbIcon: root.meta.icon
                crumbTitle: root.meta.title
                crumbSub: root.crumbSub
                newLabel: root.meta.newLabel
                showNew: root.meta.add && contentStack.depth <= 1
                showSearch: root.meta.search && contentStack.depth <= 1
                showFilter: root.meta.search && contentStack.depth <= 1
                showBack: contentStack.depth > 1
                showExport: contentStack.currentItem
                            && contentStack.currentItem.exportTable !== undefined
                            && contentStack.currentItem.exportTable !== ""
                            && contentStack.currentItem.exportId !== ""
                onBackClicked: root.goBack()
                onExportClicked: root.exportRecord(contentStack.currentItem.exportTable,
                                                   contentStack.currentItem.exportId)
                onAddClicked: root.addForCurrentSection()
                onFilterClicked: filterDialog.openFor(root.currentSection)
                onSearchEdited: (t) => {
                    switch (root.currentSection) {
                    case "projects": DesktopAppController.setQuickSearch(DesktopAppController.projectsListModel, t); break
                    case "people":   DesktopAppController.setQuickSearch(DesktopAppController.peopleModel, t); break
                    case "clients":  DesktopAppController.setQuickSearch(DesktopAppController.clientsModel, t); break
                    case "items":    DesktopAppController.setQuickSearch(DesktopAppController.allItemsModel, t); break
                    }
                }
            }

            StackView {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                initialItem: projectsComponent
                clip: true
            }
        }
    }

    // Overlay dragged sidebar rows reparent onto (always on top).
    Item {
        id: dragOverlay
        anchors.fill: parent
        z: 9999
    }

    // ── Content components ────────────────────────────────────────────────────
    Component {
        id: projectsComponent
        ProjectsListPage {
            dragLayer: dragOverlay
            selectedProjectId: root.selectedProjectId
            onProjectActivated: (pid) => root.openProject(pid)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
        }
    }
    Component {
        id: projectDetailComponent
        ProjectDetailPage {
            onNoteActivated: (noteRow, noteId) => root.openNote(noteRow, noteId, projectId)
            onItemActivated: (itemId) => root.openItem(itemId)
        }
    }
    Component {
        id: noteDetailComponent
        ProjectNoteDetailPage {}
    }
    Component {
        id: peopleComponent
        PeoplePage {
            onPersonActivated: (row, personId) => root.openPerson(row, personId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
        }
    }
    Component {
        id: personDetailComponent
        PersonDetailPage {}
    }
    Component {
        id: clientsComponent
        ClientsPage {
            onClientActivated: (row, clientId) => root.openClient(row, clientId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
        }
    }
    Component {
        id: clientDetailComponent
        ClientDetailPage {}
    }
    Component {
        id: itemsComponent
        ItemsPage {
            onItemActivated: (itemId) => root.openItem(itemId)
            onExportRequested: (table, id) => root.exportRecord(table, id)
            onFilterRequested: () => filterDialog.openFor(root.currentSection)
        }
    }
    Component {
        id: itemDetailComponent
        ItemDetailPage {}
    }
    Component {
        id: searchComponent
        SearchPage {
            onResultActivated: (dataType, dataId, fkId) => root.openSearchResult(dataType, dataId, fkId)
        }
    }
    Component {
        id: settingsComponent
        SettingsPage {}
    }
    Component {
        id: stubComponent
        StubPage {
            pageTitle: root.meta.title
            pageIcon: root.meta.icon
        }
    }

    Dialog {
        id: errorDialog
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok
        title: "Error"
        Label { id: errorLabel; wrapMode: Text.Wrap; width: 320; color: Theme.text }
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export XML to file")
        fileMode: FileDialog.SaveFile
        nameFilters: [ qsTr("XML files (*.xml)") ]
        defaultSuffix: "xml"
        onAccepted: DesktopAppController.exportRecordXml(root._exportTable, root._exportId, selectedFile)
    }

    FileDialog {
        id: importDialog
        title: qsTr("Import XML from file")
        fileMode: FileDialog.OpenFile
        nameFilters: [ qsTr("XML files (*.xml)") ]
        onAccepted: DesktopAppController.importXmlFile(selectedFile)
    }

    // Filter editor (opened from the TopBar Filter button and the hamburger menu).
    FilterDialog { id: filterDialog }
}

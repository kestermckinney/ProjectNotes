// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import ProjectNotesDesktop

// Settings screen: theme, project folders, preferences, view options, about.
Item {
    id: page

    property var _clients: []
    property var _people: []
    Component.onCompleted: {
        _clients = DesktopAppController.clientList()
        _people  = DesktopAppController.peopleList()
        mgmtCombo.value = _nameForId(_clients, DesktopAppController.managingCompanyId())
        pmCombo.value   = _nameForId(_people,  DesktopAppController.projectManagerId())
    }
    function _clientNames() { return _clients.map(function(c){ return c.name }) }
    function _peopleNames() { return _people.map(function(p){ return p.name }) }
    function _idForName(list, n) { for (var i=0;i<list.length;i++) if (list[i].name===n) return list[i].id; return "" }
    function _nameForId(list, id){ for (var i=0;i<list.length;i++) if (list[i].id===id) return list[i].name; return "" }

    ScrollView {
        id: pageScroll
        anchors.fill: parent
        anchors.margins: 14
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: pageScroll.availableWidth
            spacing: 16

            // ── Appearance ────────────────────────────────────────────────────
            SettingsSection {
                title: "Appearance"
                subtitle: "Theme used across the application."
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: [
                            { key: "system", label: "System" },
                            { key: "light",  label: "Light" },
                            { key: "dark",   label: "Dark" }
                        ]
                        delegate: Button {
                            required property var modelData
                            primary: Theme.mode === modelData.key
                            implicitHeight: 28
                            leftPadding: 12; rightPadding: 12; topPadding: 0; bottomPadding: 0
                            contentItem: Text {
                                text: modelData.label
                                color: Theme.mode === modelData.key ? "#ffffff" : Theme.text
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: Theme.mode = modelData.key
                        }
                    }
                }
            }

            // ── Cloud Sync ────────────────────────────────────────────────────
            SettingsSection {
                title: "Cloud Sync"
                subtitle: DesktopAppController.supabaseConnectionInfo

                // Status + Sync now
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    MaterialIcon {
                        name: DesktopAppController.syncNetworkError ? "cloud_off"
                              : (DesktopAppController.syncActive ? "sync" : "cloud_done")
                        size: 17
                        color: DesktopAppController.syncNetworkError ? Theme.red
                               : (DesktopAppController.syncActive ? Theme.accent : Theme.green)
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        // Progress detail — same wording as the Widgets tooltip.
                        Text {
                            Layout.fillWidth: true
                            color: DesktopAppController.syncNetworkError ? Theme.red : Theme.text
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            text: DesktopAppController.syncDetail
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: DesktopAppController.subscriptionStatusText !== ""
                            color: Theme.text3
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            textFormat: Text.RichText
                            text: DesktopAppController.subscriptionStatusText
                        }
                    }
                    Button {
                        implicitHeight: 28
                        leftPadding: 10; rightPadding: 10; topPadding: 0; bottomPadding: 0
                        enabled: DesktopAppController.syncEnabled
                        contentItem: RowLayout {
                            spacing: 4
                            MaterialIcon { name: "monitoring"; size: 14; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
                            Text { text: qsTr("Sync Stats"); color: Theme.text; font.pixelSize: 11; font.weight: Font.DemiBold
                                   verticalAlignment: Text.AlignVCenter; Layout.alignment: Qt.AlignVCenter }
                        }
                        onClicked: DesktopAppController.showSyncStats()
                    }
                    Button {
                        primary: true
                        implicitHeight: 28
                        leftPadding: 10; rightPadding: 10; topPadding: 0; bottomPadding: 0
                        enabled: DesktopAppController.syncEnabled
                        contentItem: RowLayout {
                            spacing: 4
                            MaterialIcon { name: "sync"; size: 14; color: "#ffffff"; Layout.alignment: Qt.AlignVCenter }
                            Text { text: qsTr("Sync Now"); color: "#ffffff"; font.pixelSize: 11; font.weight: Font.DemiBold
                                   verticalAlignment: Text.AlignVCenter; Layout.alignment: Qt.AlignVCenter }
                        }
                        onClicked: DesktopAppController.syncNow()
                    }
                    Button {
                        implicitHeight: 28
                        leftPadding: 10; rightPadding: 10; topPadding: 0; bottomPadding: 0
                        enabled: DesktopAppController.syncEnabled
                        contentItem: RowLayout {
                            spacing: 4
                            MaterialIcon { name: "sync_alt"; size: 14; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
                            Text { text: qsTr("Sync All"); color: Theme.text; font.pixelSize: 11; font.weight: Font.DemiBold
                                   verticalAlignment: Text.AlignVCenter; Layout.alignment: Qt.AlignVCenter }
                        }
                        onClicked: DesktopAppController.syncAll()
                    }
                }
                // thin progress line
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 3
                    radius: 2
                    visible: DesktopAppController.syncActive
                    color: Theme.surface2
                    Rectangle {
                        height: parent.height; radius: 2
                        width: parent.width * Math.max(0.03, DesktopAppController.syncProgress)
                        color: DesktopAppController.syncHasError ? Theme.red : Theme.accent
                    }
                }

                SettingsCheck {
                    label: qsTr("Enable cloud sync")
                    checked: DesktopAppController.syncEnabled
                    onToggledValue: (v) => DesktopAppController.syncEnabled = v
                }
                SyncField {
                    label: qsTr("Sync Email")
                    value: DesktopAppController.syncEmail
                    onCommitted: (v) => DesktopAppController.syncEmail = v
                }
                SyncField {
                    label: qsTr("Sync Password")
                    value: DesktopAppController.syncPassword
                    password: true
                    onCommitted: (v) => DesktopAppController.syncPassword = v
                }
                SyncField {
                    label: qsTr("Encryption Phrase")
                    value: DesktopAppController.syncEncryptionPhrase
                    password: true
                    onCommitted: (v) => DesktopAppController.syncEncryptionPhrase = v
                }
            }

            // ── Project Folders ───────────────────────────────────────────────
            SettingsSection {
                title: "Project Folders"
                subtitle: "Group projects into folders (e.g. Favorites). A project can belong to several folders. Drag projects onto a folder in the sidebar to add them."

                // New-folder row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 30
                        radius: Theme.radiusSm
                        color: Theme.surface
                        border.color: Theme.border
                        TextField {
                            id: newFolderField
                            anchors.fill: parent
                            anchors.leftMargin: 9
                            anchors.rightMargin: 9
                            placeholderText: "New folder name…"
                            color: Theme.text
                            placeholderTextColor: Theme.text3
                            background: null
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            onAccepted: page._createFolder()
                        }
                    }

                    Button {
                        primary: true
                        implicitHeight: 30
                        leftPadding: 12; rightPadding: 12; topPadding: 0; bottomPadding: 0
                        enabled: newFolderField.text.trim().length > 0
                        contentItem: RowLayout {
                            spacing: 5
                            MaterialIcon { name: "add"; size: 16; color: "#ffffff"; Layout.alignment: Qt.AlignVCenter }
                            Text { text: "Add Folder"; color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold
                                   verticalAlignment: Text.AlignVCenter; Layout.alignment: Qt.AlignVCenter }
                        }
                        onClicked: page._createFolder()
                    }
                }

                // Existing folders
                Repeater {
                    model: FolderManager.folders
                    delegate: Rectangle {
                        id: folderRow
                        required property var modelData
                        readonly property var folder: modelData
                        Layout.fillWidth: true
                        implicitHeight: 40
                        radius: Theme.radiusSm
                        color: Theme.surface
                        border.color: Theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 9
                            anchors.rightMargin: 7
                            spacing: 8

                            // Icon picker: click to open a grid of all available icons,
                            // highlighting the one currently selected.
                            Rectangle {
                                id: iconSwatch
                                width: 26; height: 26; radius: Theme.radiusSm
                                color: "transparent"
                                border.color: Theme.border
                                MaterialIcon {
                                    anchors.centerIn: parent
                                    name: folderRow.folder.icon
                                    size: 15
                                    color: folderRow.folder.color
                                }
                                TapHandler {
                                    onTapped: {
                                        var p = iconSwatch.mapToItem(null, 0, iconSwatch.height)
                                        iconPickerMenu.openFor(folderRow.folder, p.x, p.y)
                                    }
                                }
                            }

                            Text {
                                text: folderRow.folder.name
                                color: Theme.text
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: folderRow.folder.count
                                      + (folderRow.folder.count === 1 ? " project" : " projects")
                                color: Theme.text3
                                font.pixelSize: 10
                            }

                            // Color swatches
                            Row {
                                spacing: 3
                                Repeater {
                                    model: Theme.folderColors
                                    delegate: Rectangle {
                                        required property var modelData
                                        width: 14; height: 14; radius: 7
                                        color: modelData
                                        border.width: 2
                                        border.color: folderRow.folder.color === modelData
                                                      ? Theme.text : "transparent"
                                        TapHandler {
                                            onTapped: FolderManager.setFolderColor(
                                                          folderRow.folder.id, modelData)
                                        }
                                    }
                                }
                            }

                            // Delete
                            Rectangle {
                                width: 26; height: 26; radius: Theme.radiusSm
                                color: delHover.hovered ? Theme.redSoft : "transparent"
                                MaterialIcon { anchors.centerIn: parent; name: "delete"; size: 15; color: Theme.red }
                                HoverHandler { id: delHover }
                                TapHandler { onTapped: FolderManager.removeFolder(folderRow.folder.id) }
                            }
                        }
                    }
                }

                Text {
                    visible: FolderManager.folders.length === 0
                    text: "No folders yet. Add one above — it will appear in the project sidebar."
                    color: Theme.text3
                    font.pixelSize: 11
                }
            }

            // ── Preferences ───────────────────────────────────────────────────
            SettingsSection {
                title: "Preferences"
                subtitle: "Defaults applied to new projects and status reports."
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 9
                    ComboField {
                        id: mgmtCombo
                        label: qsTr("Managing Company")
                        options: page._clientNames()
                        onActivated: (v) => DesktopAppController.setManagingCompanyId(page._idForName(page._clients, v))
                    }
                    ComboField {
                        id: pmCombo
                        label: qsTr("Project Manager")
                        options: page._peopleNames()
                        onActivated: (v) => DesktopAppController.setProjectManagerId(page._idForName(page._people, v))
                    }
                }
            }

            // ── View Options ──────────────────────────────────────────────────
            SettingsSection {
                title: "View Options"
                subtitle: "Control what the lists show. Changes apply immediately."
                SettingsCheck {
                    label: qsTr("Show closed projects")
                    checked: DesktopAppController.showClosedProjects
                    onToggledValue: (v) => DesktopAppController.showClosedProjects = v
                }
                SettingsCheck {
                    label: qsTr("Show internal items")
                    checked: DesktopAppController.showInternalItems
                    onToggledValue: (v) => DesktopAppController.showInternalItems = v
                }
                SettingsCheck {
                    label: qsTr("Only New and Assigned tracker items")
                    checked: DesktopAppController.newAndAssignedOnly
                    onToggledValue: (v) => DesktopAppController.newAndAssignedOnly = v
                }
            }

            // ── Data ──────────────────────────────────────────────────────────
            SettingsSection {
                title: "Data"
                subtitle: "Import records from a Project Notes XML file. Export is available from any record's detail page."
                Button {
                    implicitHeight: 30
                    leftPadding: 12; rightPadding: 12; topPadding: 0; bottomPadding: 0
                    contentItem: RowLayout {
                        spacing: 5
                        MaterialIcon { name: "upload"; size: 16; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
                        Text { text: qsTr("Import from XML…"); color: Theme.text; font.pixelSize: 12; font.weight: Font.DemiBold
                               verticalAlignment: Text.AlignVCenter; Layout.alignment: Qt.AlignVCenter }
                    }
                    onClicked: importDialog.open()
                }
            }

            // ── About ─────────────────────────────────────────────────────────
            SettingsSection {
                title: "About"
                Text {
                    text: "Project Notes"
                    color: Theme.text; font.pixelSize: 16; font.weight: Font.Bold
                }
                Text {
                    text: "Version " + Qt.application.version + " · Built " + DesktopAppController.buildTimestamp()
                    color: Theme.text3; font.pixelSize: 12
                }
                Text {
                    text: "Qt " + DesktopAppController.qtRuntimeVersion()
                    color: Theme.text3; font.pixelSize: 12
                }
                Text {
                    visible: text.length > 0
                    text: DesktopAppController.developerProfile().length > 0
                          ? "Profile: " + DesktopAppController.developerProfile() : ""
                    color: Theme.text3; font.pixelSize: 12
                }
                Text {
                    text: "© 2022–2026 Paul McKinney"
                    color: Theme.text3; font.pixelSize: 11
                }

                RowLayout {
                    Layout.topMargin: 3
                    spacing: 6

                    Repeater {
                        model: [
                            { label: qsTr("Documentation"),  url: "https://projectnotes.readthedocs.io/" },
                            { label: qsTr("Release Notes"),  url: "https://github.com/kestermckinney/ProjectNotes/releases" },
                            { label: qsTr("Source Code"),    url: "https://github.com/kestermckinney/ProjectNotes" }
                        ]
                        delegate: Button {
                            required property var modelData
                            implicitHeight: 28
                            leftPadding: 12; rightPadding: 12; topPadding: 0; bottomPadding: 0
                            contentItem: Text {
                                text: modelData.label
                                color: Theme.text; font.pixelSize: 12; font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: Qt.openUrlExternally(modelData.url)
                        }
                    }
                }

                Rectangle {
                    Layout.topMargin: 6
                    Layout.fillWidth: true
                    Layout.preferredWidth: 420
                    implicitHeight: promoColumn.implicitHeight + 20
                    color: Theme.surface2
                    radius: Theme.radius
                    border.color: Theme.border

                    ColumnLayout {
                        id: promoColumn
                        anchors { top: parent.top; left: parent.left; right: parent.right; margins: 10 }
                        spacing: 5

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Take Project Notes With You")
                            font.pixelSize: 13; font.weight: Font.Bold
                            color: Theme.text
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Download the Project Notes mobile app for iOS to check status, review notes, and stay on top of tracker items on the go.")
                            font.pixelSize: 11
                            color: Theme.text3
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                Text {
                    Layout.topMargin: 3
                    text: "<a href=\"https://www.projectnotespro.com\">www.projectnotespro.com</a>"
                    textFormat: Text.RichText
                    font.pixelSize: 12
                    color: Theme.accent
                    linkColor: Theme.accent
                    onLinkActivated: Qt.openUrlExternally(link)
                }
            }
        }
    }

    // Inline labeled checkbox row for settings
    component SettingsCheck: RowLayout {
        id: chk
        property string label: ""
        property bool checked: false
        signal toggledValue(bool value)
        Layout.fillWidth: true
        spacing: 8
        CheckBox {
            id: cb
            checked: chk.checked
            padding: 0
            implicitWidth: 16; implicitHeight: 16
            Layout.alignment: Qt.AlignVCenter
            onToggled: chk.toggledValue(checked)
            indicator: Rectangle {
                implicitWidth: 16; implicitHeight: 16; radius: 4
                x: 0; y: cb.height/2 - height/2
                color: cb.checked ? Theme.accent : Theme.surface
                border.color: cb.checked ? Theme.accent : Theme.border
                MaterialIcon { anchors.centerIn: parent; visible: cb.checked; name: "check"; size: 12; color: "#ffffff" }
            }
            contentItem: Item {}
        }
        Text {
            text: chk.label
            color: Theme.text; font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            Layout.fillWidth: true
        }
    }

    // Labeled text input for sync settings (init once, write back on commit).
    component SyncField: ColumnLayout {
        id: sf
        property string label: ""
        property string value: ""
        property bool password: false
        signal committed(string v)
        Layout.fillWidth: true
        spacing: 3
        Text { text: sf.label; color: Theme.text3; font.pixelSize: 10; font.weight: Font.DemiBold }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 30
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: sfInput.activeFocus ? Theme.accent : Theme.border
            TextField {
                id: sfInput
                anchors.fill: parent
                anchors.leftMargin: 9; anchors.rightMargin: 9
                verticalAlignment: Text.AlignVCenter
                color: Theme.text
                background: null
                font.pixelSize: 12
                selectByMouse: true
                echoMode: sf.password ? TextInput.PasswordEchoOnEdit : TextInput.Normal
                Component.onCompleted: text = sf.value
                onEditingFinished: sf.committed(text)
            }
        }
    }

    FileDialog {
        id: importDialog
        title: qsTr("Import XML from file")
        fileMode: FileDialog.OpenFile
        nameFilters: [ qsTr("XML files (*.xml)") ]
        onAccepted: DesktopAppController.importXmlFile(selectedFile)
    }

    // Folder icon picker — a grid of Theme.folderIcons, highlighting the icon
    // currently assigned to the folder being edited. Mirrors the themed in-scene
    // Popup pattern used by SpellCheckField's context menu.
    Popup {
        id: iconPickerMenu
        modal: true
        dim: false
        padding: 8
        width: 160
        parent: Overlay.overlay
        scale: Theme.uiScale
        transformOrigin: Item.TopLeft

        property var _folder: null

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
        }

        function openFor(folder, sx, sy) {
            iconPickerMenu._folder = folder
            var maxX = (parent ? parent.width : sx + width) - width - 6
            var maxY = (parent ? parent.height : sy + 200) - 6
            x = Math.max(6, Math.min(sx, maxX))
            y = Math.max(6, Math.min(sy, maxY))
            open()
        }

        contentItem: GridLayout {
            columns: 5
            rowSpacing: 5
            columnSpacing: 5
            Repeater {
                model: Theme.folderIcons
                delegate: Rectangle {
                    id: iconCell
                    required property var modelData
                    width: 24; height: 24; radius: Theme.radiusSm
                    color: iconCellHover.hovered ? Theme.surface2 : "transparent"
                    border.width: 2
                    border.color: iconPickerMenu._folder
                                  && iconPickerMenu._folder.icon === modelData
                                  ? Theme.accent : "transparent"
                    MaterialIcon {
                        anchors.centerIn: parent
                        name: iconCell.modelData
                        size: 15
                        color: iconPickerMenu._folder ? iconPickerMenu._folder.color : Theme.text2
                    }
                    HoverHandler { id: iconCellHover }
                    TapHandler {
                        onTapped: {
                            FolderManager.setFolderIcon(iconPickerMenu._folder.id, iconCell.modelData)
                            iconPickerMenu.close()
                        }
                    }
                }
            }
        }
    }

    function _createFolder() {
        var name = newFolderField.text.trim()
        if (name.length === 0)
            return
        FolderManager.addFolder(name, "star", Theme.folderColors[0])
        newFolderField.clear()
    }

    // Inline section container
    component SettingsSection: ColumnLayout {
        property string title: ""
        property string subtitle: ""
        default property alias content: inner.data
        Layout.fillWidth: true
        spacing: 8

        Text { text: title; color: Theme.text; font.pixelSize: 14; font.weight: Font.Bold }
        Text {
            text: subtitle
            visible: subtitle !== ""
            color: Theme.text2
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        ColumnLayout {
            id: inner
            Layout.fillWidth: true
            Layout.topMargin: 3
            spacing: 6
        }
    }
}

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
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: page.width - 40
            spacing: 22

            // ── Appearance ────────────────────────────────────────────────────
            SettingsSection {
                title: "Appearance"
                subtitle: "Theme used across the application."
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Repeater {
                        model: [
                            { key: "system", label: "System" },
                            { key: "light",  label: "Light" },
                            { key: "dark",   label: "Dark" }
                        ]
                        delegate: Button {
                            required property var modelData
                            implicitHeight: 32
                            padding: 12
                            background: Rectangle {
                                radius: Theme.radiusSm
                                color: Theme.mode === modelData.key ? Theme.accent : Theme.surface2
                                border.color: Theme.border
                            }
                            contentItem: Text {
                                text: modelData.label
                                color: Theme.mode === modelData.key ? "#ffffff" : Theme.text
                                font.pixelSize: 13
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
                    spacing: 10
                    MaterialIcon {
                        name: DesktopAppController.syncNetworkError ? "cloud_off"
                              : (DesktopAppController.syncActive ? "sync" : "cloud_done")
                        size: 20
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
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            text: DesktopAppController.syncDetail
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: DesktopAppController.subscriptionStatusText !== ""
                            color: Theme.text3
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            textFormat: Text.RichText
                            text: DesktopAppController.subscriptionStatusText
                        }
                    }
                    Button {
                        implicitHeight: 32
                        padding: 12
                        enabled: DesktopAppController.syncEnabled
                        background: Rectangle {
                            radius: Theme.radiusSm
                            color: parent.enabled ? (parent.down ? Theme.accentStrong : Theme.accent) : Theme.surface2
                        }
                        contentItem: RowLayout {
                            spacing: 5
                            MaterialIcon { name: "sync"; size: 16; color: "#ffffff" }
                            Text { text: qsTr("Sync Now"); color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold }
                        }
                        onClicked: DesktopAppController.syncNow()
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
                        implicitHeight: 36
                        radius: Theme.radiusSm
                        color: Theme.surface
                        border.color: Theme.border
                        TextField {
                            id: newFolderField
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            placeholderText: "New folder name…"
                            color: Theme.text
                            placeholderTextColor: Theme.text3
                            background: null
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                            onAccepted: page._createFolder()
                        }
                    }

                    Button {
                        implicitHeight: 36
                        padding: 14
                        enabled: newFolderField.text.trim().length > 0
                        background: Rectangle {
                            radius: Theme.radiusSm
                            color: parent.enabled ? (parent.down ? Theme.accentStrong : Theme.accent)
                                                  : Theme.surface2
                        }
                        contentItem: RowLayout {
                            spacing: 6
                            MaterialIcon { name: "add"; size: 18; color: "#ffffff" }
                            Text { text: "Add Folder"; color: "#ffffff"; font.pixelSize: 13; font.weight: Font.DemiBold }
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
                        implicitHeight: 48
                        radius: Theme.radiusSm
                        color: Theme.surface
                        border.color: Theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 8
                            spacing: 10

                            // Icon picker (cycles through Theme.folderIcons)
                            Rectangle {
                                width: 30; height: 30; radius: 8
                                color: "transparent"
                                border.color: Theme.border
                                MaterialIcon {
                                    anchors.centerIn: parent
                                    name: folderRow.folder.icon
                                    size: 18
                                    color: folderRow.folder.color
                                }
                                TapHandler {
                                    onTapped: {
                                        var icons = Theme.folderIcons
                                        var i = icons.indexOf(folderRow.folder.icon)
                                        FolderManager.setFolderIcon(folderRow.folder.id,
                                            icons[(i + 1) % icons.length])
                                    }
                                }
                            }

                            Text {
                                text: folderRow.folder.name
                                color: Theme.text
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: folderRow.folder.count
                                      + (folderRow.folder.count === 1 ? " project" : " projects")
                                color: Theme.text3
                                font.pixelSize: 11
                            }

                            // Color swatches
                            Row {
                                spacing: 4
                                Repeater {
                                    model: Theme.folderColors
                                    delegate: Rectangle {
                                        required property var modelData
                                        width: 16; height: 16; radius: 8
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
                                width: 30; height: 30; radius: 8
                                color: delHover.hovered ? Theme.redSoft : "transparent"
                                MaterialIcon { anchors.centerIn: parent; name: "delete"; size: 18; color: Theme.red }
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
                    font.pixelSize: 12
                }
            }

            // ── Preferences ───────────────────────────────────────────────────
            SettingsSection {
                title: "Preferences"
                subtitle: "Defaults applied to new projects and status reports."
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 14
                    rowSpacing: 12
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
                    implicitHeight: 36
                    padding: 14
                    background: Rectangle {
                        radius: Theme.radiusSm
                        color: parent.down ? Theme.surface : Theme.surface2
                        border.color: Theme.border
                    }
                    contentItem: RowLayout {
                        spacing: 6
                        MaterialIcon { name: "upload"; size: 18; color: Theme.text2 }
                        Text { text: qsTr("Import from XML…"); color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold }
                    }
                    onClicked: importDialog.open()
                }
            }

            // ── About ─────────────────────────────────────────────────────────
            SettingsSection {
                title: "About"
                Text {
                    text: "Project Notes " + Qt.application.version
                    color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold
                }
                Text {
                    text: "New QML desktop interface. Manage your subscription at www.projectnotespro.com."
                    color: Theme.text3; font.pixelSize: 12; wrapMode: Text.WordWrap
                    Layout.fillWidth: true
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
            onToggled: chk.toggledValue(checked)
            indicator: Rectangle {
                implicitWidth: 18; implicitHeight: 18; radius: 4
                x: cb.leftPadding; y: cb.height/2 - height/2
                color: cb.checked ? Theme.accent : Theme.surface
                border.color: cb.checked ? Theme.accent : Theme.border
                MaterialIcon { anchors.centerIn: parent; visible: cb.checked; name: "check"; size: 14; color: "#ffffff" }
            }
            contentItem: Item {}
        }
        Text {
            text: chk.label
            color: Theme.text; font.pixelSize: 13
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
        spacing: 4
        Text { text: sf.label; color: Theme.text3; font.pixelSize: 11; font.weight: Font.DemiBold }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 34
            radius: Theme.radiusSm
            color: Theme.surface
            border.color: sfInput.activeFocus ? Theme.accent : Theme.border
            TextField {
                id: sfInput
                anchors.fill: parent
                anchors.leftMargin: 10; anchors.rightMargin: 10
                verticalAlignment: Text.AlignVCenter
                color: Theme.text
                background: null
                font.pixelSize: 13
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
        spacing: 10

        Text { text: title; color: Theme.text; font.pixelSize: 16; font.weight: Font.Bold }
        Text {
            text: subtitle
            visible: subtitle !== ""
            color: Theme.text2
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        ColumnLayout {
            id: inner
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 8
        }
    }
}

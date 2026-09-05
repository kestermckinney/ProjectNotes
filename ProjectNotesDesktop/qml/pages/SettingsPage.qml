// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import ProjectNotesDesktop

// Settings screen: category tabs on the left, selected settings on the right.
Item {
    id: page

    property var _clients: []
    property var _people: []
    property int currentTab: 0
    Component.onCompleted: {
        _clients = DesktopAppController.clientList()
        _people  = DesktopAppController.peopleList()
        mgmtCombo.value = _nameForId(_clients, DesktopAppController.managingCompanyId())
        pmCombo.value   = _nameForId(_people,  DesktopAppController.projectManagerId())
    }
    // Called by the shell before it navigates away from this page (the same
    // _saveNow() contract the detail pages implement). Flushes the cloud-sync
    // fields so the credential check that follows sees what the user last typed.
    function _saveNow() {
        syncEmailField.commit()
        syncPasswordField.commit()
        syncPhraseField.commit()
    }

    function _clientNames() { return _clients.map(function(c){ return c.name }) }
    function _peopleNames() { return _people.map(function(p){ return p.name }) }
    function _idForName(list, n) { for (var i=0;i<list.length;i++) if (list[i].name===n) return list[i].id; return "" }
    function _nameForId(list, id){ for (var i=0;i<list.length;i++) if (list[i].id===id) return list[i].name; return "" }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        Rectangle {
            Layout.preferredWidth: 168
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border

            ListView {
                id: tabList
                objectName: "settingsTabList"
                anchors.fill: parent
                anchors.margins: 5
                clip: true
                interactive: false
                currentIndex: page.currentTab
                model: [
                    { key: "appearance", label: qsTr("Appearance") },
                    { key: "cloudSync", label: qsTr("Cloud Sync") },
                    { key: "projectFolder", label: qsTr("Project Folder") },
                    { key: "preferences", label: qsTr("Preferences") },
                    { key: "viewOptions", label: qsTr("View Options") },
                    { key: "data", label: qsTr("Data") }
                ]

                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    objectName: "settingsTab_" + modelData.key
                    width: tabList.width
                    height: 36
                    radius: Theme.radiusSm
                    color: index === page.currentTab
                           ? Theme.accentSoft
                           : (tabHover.hovered ? Theme.surface2 : "transparent")

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        text: parent.modelData.label
                        color: parent.index === page.currentTab ? Theme.accent : Theme.text2
                        font.pixelSize: Theme.menuFont
                        font.weight: parent.index === page.currentTab ? Font.DemiBold : Font.Normal
                    }
                    HoverHandler { id: tabHover }
                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                        onTapped: page.currentTab = parent.index
                    }
                }
            }
        }

        StackLayout {
            id: settingsPages
            objectName: "settingsPages"
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: page.currentTab

            SettingsTab {
                // Appearance
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
                                font.pixelSize: Theme.fontBody
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: Theme.mode = modelData.key
                        }
                    }
                }
            }

            }

            SettingsTab {
                // Cloud Sync
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
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            text: DesktopAppController.syncDetail
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: DesktopAppController.subscriptionStatusText !== ""
                            color: Theme.text3
                            font.pixelSize: Theme.fontSm
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
                            Text { text: qsTr("Sync Stats"); color: Theme.text; font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
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
                            Text { text: qsTr("Sync Now"); color: "#ffffff"; font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
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
                            Text { text: qsTr("Sync All"); color: Theme.text; font.pixelSize: Theme.fontSm; font.weight: Font.DemiBold
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
                    id: syncEmailField
                    label: qsTr("Sync Email")
                    value: DesktopAppController.syncEmail
                    onCommitted: (v) => DesktopAppController.syncEmail = v
                }
                SyncField {
                    id: syncPasswordField
                    label: qsTr("Sync Password")
                    value: DesktopAppController.syncPassword
                    password: true
                    onCommitted: (v) => DesktopAppController.syncPassword = v
                }
                SyncField {
                    id: syncPhraseField
                    label: qsTr("Encryption Phrase")
                    value: DesktopAppController.syncEncryptionPhrase
                    password: true
                    onCommitted: (v) => DesktopAppController.syncEncryptionPhrase = v
                }
            }

            }

            SettingsTab {
                // Project Folder
                SettingsSection {
                title: "Project Folder"
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
                            font.pixelSize: Theme.fontBody
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
                            Text { text: "Add Folder"; color: "#ffffff"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
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
                                font.pixelSize: Theme.fontLg
                                font.weight: Font.DemiBold
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: folderRow.folder.count
                                      + (folderRow.folder.count === 1 ? " project" : " projects")
                                color: Theme.text3
                                font.pixelSize: Theme.fontXs
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
                    font.pixelSize: Theme.fontSm
                }
            }

            }

            SettingsTab {
                // Preferences
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

            }

            SettingsTab {
                // View Options
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

            }

            SettingsTab {
                // Data
                SettingsSection {
                title: "Data"
                subtitle: "Import records from a Project Notes XML file. Export is available from any record's detail page."
                Button {
                    implicitHeight: 30
                    leftPadding: 12; rightPadding: 12; topPadding: 0; bottomPadding: 0
                    contentItem: RowLayout {
                        spacing: 5
                        MaterialIcon { name: "upload"; size: 16; color: Theme.text2; Layout.alignment: Qt.AlignVCenter }
                        Text { text: qsTr("Import from XML…"); color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                               verticalAlignment: Text.AlignVCenter; Layout.alignment: Qt.AlignVCenter }
                    }
                    onClicked: importDialog.open()
                }
            }

            }
        }
    }

    // One independently scrollable settings category used by the StackLayout.
    component SettingsTab: ScrollView {
        id: settingsTab
        default property alias content: settingsContent.data
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: settingsContent
            width: settingsTab.availableWidth
            spacing: 16
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
            color: Theme.text; font.pixelSize: Theme.fontBody
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
        // Commit whatever is typed without waiting for focus to move — used by
        // _saveNow() so a value the user typed and then navigated away from is
        // saved (and checked) rather than lost. Committing an unchanged value is
        // harmless: the controller's setters ignore it.
        function commit() { sf.committed(sfInput.text) }
        Layout.fillWidth: true
        spacing: 3
        Text { text: sf.label; color: Theme.text3; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
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
                font.pixelSize: Theme.fontBody
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

        // Clicking away dismisses the picker and nothing else — see ClickShield.qml.
        ClickShield { host: iconPickerMenu }

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

        Text { text: title; color: Theme.text; font.pixelSize: Theme.fontXl; font.weight: Font.Bold }
        Text {
            text: subtitle
            visible: subtitle !== ""
            color: Theme.text2
            font.pixelSize: Theme.fontBody
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

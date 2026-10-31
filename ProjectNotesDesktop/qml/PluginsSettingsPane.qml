// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ProjectNotesDesktop

// Settings > Plugins: installed JavaScript plugins, enabling them (with a
// review of what a user plugin asks for), installing .pnplugin packages or
// plugin folders, opening the user plugin folder, and backing up plugin
// settings to a JSON file to restore here or on another computer.
ColumnLayout {
    id: pane
    objectName: "pluginsSettingsPane"
    spacing: 12
    property string error: ""
    property string backupStatus: ""
    // Plugins with saved settings, and the ones ticked for export (id -> bool).
    property var backupCandidates: []
    property var backupChecked: ({})

    function refreshBackupCandidates() {
        var list = PluginUiController.settingsBackupCandidates()
        var checked = {}
        list.forEach(function (c) { checked[c.id] = pane.backupChecked[c.id] !== false })
        backupChecked = checked
        backupCandidates = list
    }
    function selectedBackupIds() {
        return backupCandidates.filter(function (c) { return backupChecked[c.id] }).map(function (c) { return c.id })
    }
    Component.onCompleted: refreshBackupCandidates()
    Connections {
        target: PluginUiController
        function onPluginsChanged() { pane.refreshBackupCandidates() }
        function onSettingsRevisionChanged() { pane.refreshBackupCandidates() }
    }

    // Plain-language list of what a plugin can do, grouped for display. Each item
    // carries a group heading and a flag for the permissions a user should look at
    // twice (changing data, starting programs, Windows automation, shipped QML).
    function permissionItems(p) {
        var items = []
        function add(group, text, warn) { items.push({ group: group, text: text, warn: !!warn }) }
        ;(p.permissions || []).forEach(function (perm) {
            if (perm.indexOf("data.read:") === 0) add(qsTr("Data"), qsTr("Read: %1").arg(perm.slice(10).split(",").join(", ")))
            else if (perm.indexOf("data.write:") === 0) add(qsTr("Data"), qsTr("Read and change: %1").arg(perm.slice(11).split(",").join(", ")), true)
            else if (perm === "http") add(qsTr("Network"), qsTr("Internet access to: %1").arg((p.httpHosts || []).join(", ")))
            else if (perm === "office365") add(qsTr("Network"), qsTr("Office 365 (%1)").arg((p.office365Scopes || []).join(", ")))
            else if (perm === "files") add(qsTr("Files and system"), qsTr("Files and folders you choose, and project folders"))
            else if (perm === "process.launch") add(qsTr("Files and system"), qsTr("Start other programs"), true)
            else if (perm === "com") add(qsTr("Files and system"), qsTr("Windows automation of: %1").arg((p.comObjects || []).join(", ")), true)
            else if (perm === "clipboard") add(qsTr("Files and system"), qsTr("Copy to and read from the clipboard"))
            else if (perm === "urls") add(qsTr("Files and system"), qsTr("Open links that start other applications"))
            else if (perm === "auth.browser") add(qsTr("Sign-in"), qsTr("Sign you in through your web browser"))
            else if (perm === "email.compose") add(qsTr("Email and calendar"), qsTr("Prepare email drafts for your review"))
            else if (perm === "calendar.draft") add(qsTr("Email and calendar"), qsTr("Prepare meeting drafts for your review"))
            else if (perm === "contacts.pick") add(qsTr("Contacts"), qsTr("Ask you to choose contacts"))
            else if (perm === "contacts.sync") add(qsTr("Contacts"), qsTr("Start contact syncs"))
            else if (perm === "ui.dialogs") add(qsTr("Interface"), qsTr("Show its own dialogs"))
            else if (perm === "app.navigate") add(qsTr("Interface"), qsTr("Open pages in Project Notes"))
            else add(qsTr("Other"), perm)
        })
        if (p.shipsQml) add(qsTr("Interface"), qsTr("Includes its own user interface (QML), which runs with the same trust as the app"), true)
        return items
    }

    function permissionText(p) {
        var lines = permissionItems(p).map(function (item) { return item.text })
        return lines.length ? lines.join("\n") : qsTr("No special permissions")
    }

    function escapeHtml(s) {
        return String(s === undefined || s === null ? "" : s)
            .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;")
    }

    // Rich-text body for the install and review dialogs: a header, the plugin's
    // description, then its permissions grouped under small headings.
    function detailsHtml(p, intro) {
        var muted = Theme.text2
        var html = ""
        if (intro) html += "<p style=\"color:" + Theme.amber + "\">" + escapeHtml(intro) + "</p>"
        var byline = p.version ? qsTr("Version %1").arg(escapeHtml(p.version)) : ""
        if (p.author) byline += (byline ? " · " : "") + qsTr("by %1").arg(escapeHtml(p.author))
        if (byline) html += "<p style=\"color:" + muted + "\">" + byline + "</p>"
        if (p.description) html += "<p>" + escapeHtml(p.description) + "</p>"

        var items = permissionItems(p)
        if (!items.length) {
            html += "<p style=\"color:" + muted + "\">" + qsTr("No special permissions") + "</p>"
            return html
        }
        html += "<p><b>" + qsTr("It will be able to:") + "</b></p>"
        var group = ""
        items.forEach(function (item) {
            if (item.group !== group) {
                if (group) html += "</ul>"
                group = item.group
                html += "<p style=\"margin-top:8px; color:" + muted + "\">" + escapeHtml(group) + "</p><ul style=\"margin-top:0\">"
            }
            var text = escapeHtml(item.text)
            if (item.warn) text = "<span style=\"color:" + Theme.amber + "\">" + text + "</span>"
            html += "<li>" + text + "</li>"
        })
        html += "</ul>"
        return html
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Theme.text2
        text: qsTr("Plugins add menus, dialogs and settings to Project Notes. Plugins you install go in your plugins folder and stay off until you review what they can do.")
    }

    RowLayout {
        spacing: 8
        Button {
            objectName: "installPluginButton"
            text: qsTr("Install Plugin…")
            enabled: PluginUiController.userPluginsAllowed
            onClicked: packagePicker.open()
        }
        Button {
            text: qsTr("Install from Folder…")
            enabled: PluginUiController.userPluginsAllowed
            onClicked: folderPicker.open()
        }
        Button {
            text: qsTr("Open Plugins Folder")
            enabled: PluginUiController.userPluginsAllowed
            onClicked: PluginUiController.openPluginsFolder()
        }
    }
    Label {
        Layout.fillWidth: true
        visible: PluginUiController.userPluginsPath !== ""
        text: qsTr("Plugins folder: %1").arg(PluginUiController.userPluginsPath)
        color: Theme.text3; font.pixelSize: Theme.fontSm; wrapMode: Text.WrapAnywhere
    }
    Label { visible: pane.error !== ""; text: pane.error; color: Theme.red; wrapMode: Text.Wrap; Layout.fillWidth: true }

    Label {
        visible: PluginUiController.plugins.length === 0
        text: qsTr("No plugins are installed.")
        color: Theme.text3
    }

    Repeater {
        model: PluginUiController.plugins
        delegate: Rectangle {
            id: card
            required property var modelData
            readonly property var p: modelData
            Layout.fillWidth: true
            implicitHeight: cardColumn.implicitHeight + 20
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border

            ColumnLayout {
                id: cardColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: card.p.name
                        color: Theme.text; font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label { text: card.p.version ? "v" + card.p.version : ""; color: Theme.text3; font.pixelSize: Theme.fontSm }
                    Rectangle {
                        radius: Theme.radiusSm
                        color: card.p.bundled ? Theme.surface2 : Theme.accentSoft
                        implicitWidth: badge.implicitWidth + 12
                        implicitHeight: badge.implicitHeight + 4
                        Label { id: badge; anchors.centerIn: parent; text: card.p.bundled ? qsTr("Included") : qsTr("Installed"); color: Theme.text2; font.pixelSize: Theme.fontXs }
                    }
                }
                Label {
                    visible: !!card.p.author
                    text: qsTr("by %1").arg(card.p.author || "")
                    color: Theme.text3; font.pixelSize: Theme.fontSm
                    Layout.fillWidth: true; elide: Text.ElideRight
                }
                Label {
                    visible: !!card.p.description
                    text: card.p.description || ""
                    color: Theme.text2; font.pixelSize: Theme.fontBody
                    wrapMode: Text.Wrap; lineHeight: 1.25
                    Layout.fillWidth: true
                    Layout.topMargin: 4; Layout.bottomMargin: 4
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSm
                    color: !card.p.valid || card.p.lastError ? Theme.red : (card.p.needsApproval ? Theme.amber : Theme.text3)
                    text: !card.p.valid ? qsTr("Cannot load: %1").arg(card.p.loadError)
                        : !card.p.platformSupported ? qsTr("Not available on this platform.")
                        : card.p.needsApproval ? qsTr("Off until you review its permissions.")
                        : card.p.lastError ? qsTr("Error: %1").arg(card.p.lastError)
                        : card.p.running ? qsTr("Running") : (card.p.enabled ? qsTr("Starting…") : qsTr("Turned off"))
                }
                RowLayout {
                    spacing: 8
                    Button {
                        visible: card.p.valid && card.p.needsApproval
                        text: qsTr("Review and Turn On…")
                        primary: true
                        onClicked: approval.openFor(card.p)
                    }
                    Button {
                        visible: card.p.valid && !card.p.needsApproval && card.p.platformSupported
                        text: card.p.enabled ? qsTr("Turn Off") : qsTr("Turn On")
                        onClicked: pane.error = PluginUiController.setEnabled(card.p.id, !card.p.enabled)
                    }
                    Button {
                        visible: card.p.enabled
                        text: qsTr("Reload")
                        onClicked: PluginUiController.reload(card.p.id)
                    }
                    Button {
                        text: qsTr("Permissions")
                        visible: card.p.valid
                        onClicked: approval.openFor(card.p, true)
                    }
                    Button {
                        visible: !card.p.bundled
                        text: qsTr("Remove")
                        onClicked: removeConfirm.openFor(card.p)
                    }
                }
            }
        }
    }

    // Settings backup: export plugin settings to a JSON file and import them
    // here or on another computer. Also reads the Python Settings Migrator files.
    Rectangle {
        objectName: "pluginSettingsBackup"
        Layout.fillWidth: true
        Layout.topMargin: 8
        implicitHeight: backupColumn.implicitHeight + 20
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border

        ColumnLayout {
            id: backupColumn
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6
            Label {
                text: qsTr("Back Up Plugin Settings")
                color: Theme.text; font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: Theme.text2; font.pixelSize: Theme.fontBody
                text: qsTr("Save plugin settings to a file, then import the file here or on another computer. Passwords are not included; they stay in your system's credential store. Files saved by the old Settings Migrator can be imported too.")
            }
            Label {
                visible: pane.backupCandidates.length === 0
                text: qsTr("No plugin has saved settings yet.")
                color: Theme.text3; font.pixelSize: Theme.fontSm
            }
            Repeater {
                model: pane.backupCandidates
                delegate: ThemedCheckBox {
                    required property var modelData
                    Layout.fillWidth: true
                    text: qsTr("%1 (%n setting(s))", "", modelData.count).arg(modelData.name)
                    checked: pane.backupChecked[modelData.id] === true
                    onToggled: {
                        var next = Object.assign({}, pane.backupChecked)
                        next[modelData.id] = checked
                        pane.backupChecked = next
                    }
                }
            }
            RowLayout {
                spacing: 8
                Layout.topMargin: 4
                Button {
                    objectName: "exportPluginSettingsButton"
                    text: qsTr("Export Settings…")
                    enabled: pane.selectedBackupIds().length > 0
                    onClicked: exportPicker.open()
                }
                Button {
                    objectName: "importPluginSettingsButton"
                    text: qsTr("Import Settings…")
                    onClicked: importPicker.open()
                }
            }
            Label {
                visible: pane.backupStatus !== ""
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: pane.backupStatus
                color: Theme.text3; font.pixelSize: Theme.fontSm
            }
        }
    }

    FileDialog {
        id: exportPicker
        title: qsTr("Export Plugin Settings")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("JSON files (*.json)")]
        onAccepted: {
            pane.error = PluginUiController.exportSettings(selectedFile, pane.selectedBackupIds())
            pane.backupStatus = pane.error === ""
                ? qsTr("Saved to %1.").arg(PluginUiController.urlToPath(selectedFile)) : ""
        }
    }
    FileDialog {
        id: importPicker
        title: qsTr("Import Plugin Settings")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("JSON files (*.json)"), qsTr("All files (*)")]
        onAccepted: importConfirm.openFor(selectedFile)
    }

    ThemedMessageDialog {
        id: importConfirm
        property url source
        iconName: "settings_backup_restore"
        iconColor: Theme.accent
        confirmText: qsTr("Import")
        function openFor(url) {
            source = url
            title = qsTr("Import Plugin Settings")
            message = qsTr("Settings in %1 replace the current values of the same settings. Other settings are kept.")
                          .arg(PluginUiController.urlToPath(url))
            open()
        }
        onConfirmed: {
            var result = PluginUiController.importSettings(source)
            pane.error = result.ok ? "" : result.error
            pane.backupStatus = result.ok
                ? qsTr("Imported %n setting(s) for %1.", "", result.count).arg(result.plugins.join(", ")) : ""
        }
    }

    FileDialog {
        id: packagePicker
        title: qsTr("Install Plugin")
        nameFilters: [qsTr("Project Notes plugins (*.pnplugin *.zip)"), qsTr("All files (*)")]
        onAccepted: pane.reviewPackage(selectedFile)
    }
    FolderDialog {
        id: folderPicker
        title: qsTr("Install Plugin from Folder")
        onAccepted: pane.reviewPackage(selectedFolder)
    }

    function reviewPackage(url) {
        var info = PluginUiController.inspect(url)
        if (!info.ok) { pane.error = info.error; return }
        pane.error = ""
        install.openFor(info, url)
    }

    // Install confirmation: what the plugin will be able to do.
    ThemedMessageDialog {
        id: install
        property var info: ({})
        property url source
        iconName: "extension"
        iconColor: Theme.accent
        confirmText: qsTr("Install")
        richMessage: true
        preferredWidth: 600
        function openFor(i, url) {
            info = i
            source = url
            title = qsTr("Install %1").arg(i.name)
            message = pane.detailsHtml(i, i.replaces ? qsTr("This replaces the installed copy.") : "")
            open()
        }
        onConfirmed: {
            pane.error = PluginUiController.install(source)
            if (pane.error === "") pane.error = PluginUiController.approve(info.id)
        }
    }

    // Review (and approve) an installed plugin's permissions.
    ThemedMessageDialog {
        id: approval
        property var plugin: ({})
        iconName: "verified_user"
        iconColor: Theme.accent
        richMessage: true
        preferredWidth: 600
        function openFor(p, readOnly) {
            plugin = p
            title = p.name
            confirmText = readOnly ? "" : qsTr("Turn On")
            message = pane.detailsHtml(p, "")
            open()
        }
        onConfirmed: pane.error = PluginUiController.approve(plugin.id)
    }

    ThemedMessageDialog {
        id: removeConfirm
        property var plugin: ({})
        iconName: "delete"
        confirmText: qsTr("Remove")
        destructive: true
        function openFor(p) {
            plugin = p
            title = qsTr("Remove %1").arg(p.name)
            message = qsTr("Delete this plugin from your plugins folder? Its settings are kept.")
            open()
        }
        onConfirmed: pane.error = PluginUiController.uninstall(plugin.id)
    }
}

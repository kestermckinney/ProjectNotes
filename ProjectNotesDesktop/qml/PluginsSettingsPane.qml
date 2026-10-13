// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ProjectNotesDesktop

// Settings > Plugins: installed JavaScript plugins, enabling them (with a
// review of what a user plugin asks for), installing .pnplugin packages or
// plugin folders, and opening the user plugin folder.
ColumnLayout {
    id: pane
    objectName: "pluginsSettingsPane"
    spacing: 12
    property string error: ""

    function permissionText(p) {
        var lines = []
        ;(p.permissions || []).forEach(function (perm) {
            if (perm.indexOf("data.read:") === 0) lines.push(qsTr("Read: %1").arg(perm.slice(10).split(",").join(", ")))
            else if (perm.indexOf("data.write:") === 0) lines.push(qsTr("Read and change: %1").arg(perm.slice(11).split(",").join(", ")))
            else if (perm === "files") lines.push(qsTr("Files and folders you choose, and project folders"))
            else if (perm === "http") lines.push(qsTr("Internet access to: %1").arg((p.httpHosts || []).join(", ")))
            else if (perm === "com") lines.push(qsTr("Windows automation of: %1").arg((p.comObjects || []).join(", ")))
            else if (perm === "office365") lines.push(qsTr("Office 365 (%1)").arg((p.office365Scopes || []).join(", ")))
            else if (perm === "email.compose") lines.push(qsTr("Prepare email drafts for your review"))
            else if (perm === "calendar.draft") lines.push(qsTr("Prepare meeting drafts for your review"))
            else if (perm === "process.launch") lines.push(qsTr("Start other programs"))
            else if (perm === "ui.dialogs") lines.push(qsTr("Show its own dialogs"))
            else if (perm === "clipboard") lines.push(qsTr("Copy to and read from the clipboard"))
            else if (perm === "urls") lines.push(qsTr("Open links that start other applications"))
            else if (perm === "auth.browser") lines.push(qsTr("Sign you in through your web browser"))
            else if (perm === "contacts.pick") lines.push(qsTr("Ask you to choose contacts"))
            else if (perm === "contacts.sync") lines.push(qsTr("Start contact syncs"))
            else if (perm === "app.navigate") lines.push(qsTr("Open pages in Project Notes"))
            else lines.push(perm)
        })
        if (p.shipsQml) lines.push(qsTr("Includes its own user interface (QML), which runs with the same trust as the app"))
        return lines.length ? lines.join("\n") : qsTr("No special permissions")
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
                Label { visible: !!card.p.description; text: card.p.description || ""; color: Theme.text2; wrapMode: Text.Wrap; Layout.fillWidth: true }
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
        function openFor(i, url) {
            info = i
            source = url
            title = qsTr("Install %1").arg(i.name)
            message = (i.replaces ? qsTr("This replaces the installed copy.\n\n") : "")
                + qsTr("Version %1%2\n\nIt will be able to:\n%3")
                    .arg(i.version || "?").arg(i.author ? qsTr(" by %1").arg(i.author) : "")
                    .arg(pane.permissionText(i))
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
        function openFor(p, readOnly) {
            plugin = p
            title = p.name
            confirmText = readOnly ? "" : qsTr("Turn On")
            message = qsTr("%1 can:\n%2").arg(p.name).arg(pane.permissionText(p))
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

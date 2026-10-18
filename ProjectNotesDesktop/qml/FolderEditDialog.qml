// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// New / edit dialog for one project folder: name, icon and color. Opened from
// the sidebar's folder-header menu; Settings ▸ Project Folders edits the same
// fields in place.
//   folderEditDialog.openNew()
//   folderEditDialog.openFor(folder)   // a FolderManager.folders entry
Dialog {
    id: dlg
    objectName: "folderEditDialog"
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 300
    height: content.implicitHeight + topPadding + bottomPadding
    modal: true; padding: 0
    scale: Theme.uiScale
    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

    ClickShield { host: dlg }

    property string _folderId: ""          // "" = creating a new folder
    property string _icon: "folder"
    property string _color: Theme.folderColors[0]
    property string _error: ""

    function openNew() {
        _folderId = ""
        nameField.text = ""
        _icon = "star"
        _color = Theme.folderColors[0]
        _error = ""
        open()
        nameField.forceActiveFocus()
    }

    function openFor(folder) {
        _folderId = folder.id
        nameField.text = folder.name
        _icon = folder.icon
        _color = folder.color
        _error = ""
        open()
        nameField.forceActiveFocus()
        nameField.selectAll()
    }

    function _save() {
        var name = nameField.text.trim()
        _error = FolderManager.folderNameError(name, _folderId)
        if (_error !== "")
            return
        if (_folderId === "") {
            FolderManager.addFolder(name, _icon, _color)
        } else {
            FolderManager.renameFolder(_folderId, name)
            FolderManager.setFolderIcon(_folderId, _icon)
            FolderManager.setFolderColor(_folderId, _color)
        }
        close()
    }

    contentItem: ColumnLayout {
        id: content
        spacing: 0
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 12
            Text {
                text: dlg._folderId === "" ? qsTr("New Folder") : qsTr("Edit Folder")
                color: Theme.text; font.pixelSize: Theme.fontXl; font.weight: Font.Bold
                Layout.fillWidth: true
            }
            DialogCloseButton { popup: dlg; iconSize: 18 }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 8

            Text { text: qsTr("Name"); color: Theme.text2; font.pixelSize: Theme.fontSm }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 30
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: nameField.activeFocus ? Theme.accent : Theme.border
                TextField {
                    id: nameField
                    objectName: "folderEditName"
                    anchors.fill: parent
                    anchors.leftMargin: 9; anchors.rightMargin: 9
                    color: Theme.text
                    background: null
                    font.pixelSize: Theme.fontBody
                    verticalAlignment: Text.AlignVCenter
                    onTextEdited: dlg._error = ""
                    onAccepted: dlg._save()
                    SpellCheckField { spellCheckEnabled: false }
                }
            }
            Text {
                visible: dlg._error !== ""
                text: dlg._error
                color: Theme.red
                font.pixelSize: Theme.fontSm
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            Text { text: qsTr("Color"); color: Theme.text2; font.pixelSize: Theme.fontSm; Layout.topMargin: 4 }
            Flow {
                Layout.fillWidth: true
                spacing: 5
                Repeater {
                    model: Theme.folderColors
                    delegate: Rectangle {
                        required property var modelData
                        width: 18; height: 18; radius: 9
                        color: modelData
                        border.width: 2
                        border.color: dlg._color === modelData ? Theme.text : "transparent"
                        TapHandler { onTapped: dlg._color = modelData }
                    }
                }
            }

            Text { text: qsTr("Icon"); color: Theme.text2; font.pixelSize: Theme.fontSm; Layout.topMargin: 4 }
            Flow {
                Layout.fillWidth: true
                spacing: 5
                Repeater {
                    model: Theme.folderIcons
                    delegate: Rectangle {
                        id: iconCell
                        required property var modelData
                        width: 26; height: 26; radius: Theme.radiusSm
                        color: iconHover.hovered ? Theme.surface2 : "transparent"
                        border.width: 2
                        border.color: dlg._icon === modelData ? Theme.accent : "transparent"
                        MaterialIcon { anchors.centerIn: parent; name: iconCell.modelData; size: 15; color: dlg._color }
                        HoverHandler { id: iconHover }
                        TapHandler { onTapped: dlg._icon = iconCell.modelData }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                spacing: 8
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Cancel")
                    implicitHeight: 30
                    onClicked: dlg.close()
                }
                Button {
                    primary: true
                    objectName: "folderEditSave"
                    text: dlg._folderId === "" ? qsTr("Add Folder") : qsTr("Save")
                    implicitHeight: 30
                    enabled: nameField.text.trim().length > 0
                    onClicked: dlg._save()
                }
            }
        }
    }
}

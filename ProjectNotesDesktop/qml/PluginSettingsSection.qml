// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// One Settings section a plugin contributes (plugin.json contributes.settings):
// a form schema rendered with SchemaForm, saved as each value changes, or the
// plugin's own QML with the `pluginSettings` context object.
ColumnLayout {
    id: section
    property var info: ({})
    spacing: 10

    Label {
        text: section.info.title || ""
        color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
    }
    Label {
        visible: !!section.info.pluginName && section.info.pluginName !== section.info.title
        text: qsTr("From %1").arg(section.info.pluginName || "")
        color: Theme.text3; font.pixelSize: Theme.fontSm
    }

    SchemaForm {
        id: form
        visible: !!section.info.schema
        Layout.fillWidth: true
        fields: section.info.schema && section.info.schema.fields ? section.info.schema.fields : []
        values: section.info.pluginId ? PluginUiController.settingValues(section.info.pluginId) : ({})
        onChanged: (key, value) => PluginUiController.setSetting(section.info.pluginId, key, value)
    }

    Item {
        id: qmlHolder
        visible: !!section.info.qml
        Layout.fillWidth: true
        implicitHeight: childrenRect.height
        Component.onCompleted: {
            if (!section.info.qml) return
            var item = PluginUiController.createSettingsContent(section.info.pluginId, section.info.qml, qmlHolder)
            if (item) item.width = Qt.binding(function () { return qmlHolder.width })
        }
    }
}

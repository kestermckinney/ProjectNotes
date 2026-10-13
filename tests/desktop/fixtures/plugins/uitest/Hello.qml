// A plugin QML dialog: uses the app's controls through ProjectNotes.PluginUI,
// calls back into the plugin with pluginDialog.invoke, then accepts.
import QtQuick
import QtQuick.Layouts
import ProjectNotes.PluginUI

ColumnLayout {
    FormField { Layout.fillWidth: true; label: pluginDialog.title; text: pluginDialog.data.greeting }
    Component.onCompleted: {
        pluginDialog.invoke("shout", [pluginDialog.data.greeting]).then(function (loud) {
            pluginDialog.accept(loud + " back")
        })
    }
}

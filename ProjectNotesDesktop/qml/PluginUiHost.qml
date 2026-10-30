// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ProjectNotesDesktop

// Shows every dialog a JavaScript plugin asks for (PluginUiController
// requestOpened): messages, confirmations, prompts, forms, plugin QML dialogs,
// file pickers, progress, the contact and recipient pickers, and the meeting
// review step.
// Plugin emails skip review and go straight to a draft. Each dialog answers through PluginUiController.respond(); closing it
// any other way cancels (null). Popups use Theme.uiScale like the rest of the
// app's dialogs, so zoom applies to plugin UI too.
Item {
    id: host
    anchors.fill: parent
    property var _open: ({})

    Connections {
        target: PluginUiController
        function onRequestOpened(requestId, kind, params) { host.openRequest(requestId, kind, params) }
        function onRequestClosed(requestId) {
            var p = host._open[requestId]
            if (p) p.dismiss()
        }
        function onProgressUpdated(token, fraction, text) {
            for (var id in host._open) {
                var p = host._open[id]
                if (p && p.params && p.params.token === token && p.updateProgress) p.updateProgress(fraction, text)
            }
        }
        function onNotificationRequested(text, kind, seconds) { toast.show(text, kind, seconds) }
        function onPluginError(id, name, message) { toast.show((name || id) + ": " + message, "error", 8) }
    }

    function openRequest(requestId, kind, params) {
        var component = null
        var props = { requestId: requestId, params: params }
        switch (kind) {
        case "message":
        case "confirm": component = messageComponent; break
        case "prompt":
            component = formComponent
            props.fields = [{ key: "value", label: params.label || "", required: false,
                              type: params.password ? "password" : (params.multiline ? "multiline" : "text"),
                              placeholder: params.placeholder || "" }]
            props.initialValues = { value: params.value || "" }
            props.transform = function (values) { return values.value === undefined ? "" : values.value }
            break
        case "choose":
            component = formComponent
            if (params.multiple) {
                props.fields = (params.label ? [{ key: "_label", label: params.label, type: "note" }] : [])
                    .concat((params.choices || []).map(function (c, i) { return { key: "c" + i, label: String(c), type: "checkbox" } }))
                props.transform = function (values) {
                    return (params.choices || []).filter(function (c, i) { return !!values["c" + i] })
                }
            } else {
                props.fields = [{ key: "value", label: params.label || "", type: "choice", required: true, choices: params.choices || [] }]
                props.transform = function (values) { return values.value === undefined ? null : values.value }
            }
            props.initialValues = {}
            break
        case "form":
            component = formComponent
            props.fields = (params.form && params.form.fields) ? params.form.fields : []
            props.initialValues = params.values || {}
            break
        case "dialog": component = dialogComponent; break
        case "pickFile":
        case "saveFile": component = fileComponent; break
        case "pickFolder": component = folderComponent; break
        case "progress": component = progressComponent; break
        case "contactPicker": component = contactComponent; break
        case "recipientPicker": component = recipientComponent; break
        case "meeting": component = meetingComponent; break
        }
        if (!component) {
            PluginUiController.logRequestProblem(requestId, "no dialog handles this kind of request")
            PluginUiController.cancelRequest(requestId)
            return
        }
        var popup = component.createObject(host, props)
        if (!popup) {
            PluginUiController.logRequestProblem(requestId, "the dialog could not be created: " + component.errorString())
            PluginUiController.cancelRequest(requestId)
            return
        }
        popup.objectName = "pluginRequest_" + kind
        _open[requestId] = popup
        popup.open()
        if (popup.visible === false)
            PluginUiController.logRequestProblem(requestId, "the dialog did not become visible")
    }

    function _forget(requestId) { delete _open[requestId] }
    function _fileUrl(path) { return "file:///" + String(path).replace(/\\/g, "/").replace(/^\/+/, "") }

    // ── Shared dialog chrome ───────────────────────────────────────────────
    component PluginDialog: Dialog {
        id: dlg
        property int requestId: 0
        property var params: ({})
        property bool answered: false
        property string iconName: "extension"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 16
        scale: Theme.uiScale
        closePolicy: Popup.CloseOnEscape
        title: params.title || params.pluginName || ""
        background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
        header: Item {
            implicitHeight: 48
            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8
                MaterialIcon { name: dlg.iconName; size: 20; color: Theme.accent }
                Label {
                    text: dlg.title
                    color: Theme.text; font.pixelSize: Theme.font2xl; font.weight: Font.Bold
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Label {
                    visible: !!dlg.params.pluginName && dlg.params.pluginName !== dlg.title
                    text: dlg.params.pluginName || ""
                    color: Theme.text3; font.pixelSize: Theme.fontSm
                }
            }
        }
        function answer(result) {
            if (answered) return
            answered = true
            PluginUiController.respond(requestId, result)
            close()
        }
        function dismiss() {
            answered = true
            close()
        }
        onClosed: {
            if (!answered) PluginUiController.cancelRequest(requestId)
            host._forget(requestId)
            destroy()
        }
    }

    // ── message / confirm ──────────────────────────────────────────────────
    Component {
        id: messageComponent
        PluginDialog {
            id: msg
            width: 420
            readonly property bool confirming: params.okText !== undefined
            iconName: params.kind === "error" ? "error" : params.kind === "warning" ? "warning" : (confirming ? "help" : "info")
            contentItem: Label {
                text: msg.params.text || ""
                color: Theme.text
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
            }
            footer: DialogButtonRow {
                Button { visible: msg.confirming; text: msg.params.cancelText || qsTr("Cancel"); onClicked: msg.answer(false) }
                Button {
                    text: msg.confirming ? (msg.params.okText || qsTr("OK")) : qsTr("OK")
                    primary: true
                    onClicked: msg.answer(msg.confirming ? true : null)
                }
            }
        }
    }

    // ── form / prompt / choose ─────────────────────────────────────────────
    Component {
        id: formComponent
        PluginDialog {
            id: formDialog
            property var fields: []
            property var initialValues: ({})
            property var transform: null
            property string error: ""
            width: Math.min(params.width || 460, host.width - 40)
            iconName: "edit_note"
            contentItem: ScrollView {
                id: scroll
                implicitHeight: Math.min(form.implicitHeight, host.height * 0.6)
                contentWidth: availableWidth
                clip: true
                ColumnLayout {
                    width: scroll.availableWidth
                    spacing: 8
                    SchemaForm {
                        id: form
                        Layout.fillWidth: true
                        fields: formDialog.fields
                        values: formDialog.initialValues
                    }
                    Label { visible: formDialog.error !== ""; text: formDialog.error; color: Theme.red; wrapMode: Text.Wrap; Layout.fillWidth: true }
                }
            }
            footer: DialogButtonRow {
                Button { text: qsTr("Cancel"); onClicked: formDialog.answer(null) }
                Button {
                    text: formDialog.params.okText || qsTr("OK")
                    primary: true
                    onClicked: {
                        var missing = form.missingRequired()
                        if (missing !== "") { formDialog.error = qsTr("%1 is required.").arg(missing); return }
                        var values = form.collect()
                        formDialog.answer(formDialog.transform ? formDialog.transform(values) : values)
                    }
                }
            }
        }
    }

    // ── plugin QML dialog ──────────────────────────────────────────────────
    Component {
        id: dialogComponent
        PluginDialog {
            id: pluginDialogPopup
            width: Math.min(params.width || 520, host.width - 40)
            height: Math.min(params.height || 400, host.height - 40)
            padding: 12
            contentItem: Item {
                id: holder
                Component.onCompleted: {
                    var item = PluginUiController.createDialogContent(pluginDialogPopup.requestId, holder)
                    if (item) item.anchors.fill = holder
                    else pluginDialogPopup.dismiss()   // the request was answered with the load error
                }
            }
        }
    }

    // ── file and folder pickers ────────────────────────────────────────────
    Component {
        id: fileComponent
        FileDialog {
            id: fileDialog
            property int requestId: 0
            property var params: ({})
            property bool answered: false
            title: params.title || ""
            fileMode: params.multiple ? FileDialog.OpenFiles
                     : (params.suggestedName !== undefined ? FileDialog.SaveFile : FileDialog.OpenFile)
            nameFilters: params.filters && params.filters.length ? params.filters : [qsTr("All files (*)")]
            Component.onCompleted: {
                if (params.startIn) currentFolder = host._fileUrl(params.startIn)
                if (params.suggestedName)
                    selectedFile = host._fileUrl(String(params.suggestedName).indexOf("/") >= 0 || !params.startIn
                                                 ? params.suggestedName : params.startIn + "/" + params.suggestedName)
            }
            function dismiss() { answered = true; close() }
            onAccepted: {
                answered = true
                if (params.multiple)
                    PluginUiController.respond(requestId, selectedFiles.map(function (u) { return PluginUiController.urlToPath(u) }))
                else
                    PluginUiController.respond(requestId, PluginUiController.urlToPath(selectedFile))
                host._forget(requestId)
                destroy()
            }
            onRejected: {
                if (!answered) PluginUiController.cancelRequest(requestId)
                host._forget(requestId)
                destroy()
            }
        }
    }
    Component {
        id: folderComponent
        FolderDialog {
            property int requestId: 0
            property var params: ({})
            property bool answered: false
            title: params.title || ""
            Component.onCompleted: if (params.startIn) currentFolder = host._fileUrl(params.startIn)
            function dismiss() { answered = true; close() }
            onAccepted: {
                answered = true
                PluginUiController.respond(requestId, PluginUiController.urlToPath(selectedFolder))
                host._forget(requestId)
                destroy()
            }
            onRejected: {
                if (!answered) PluginUiController.cancelRequest(requestId)
                host._forget(requestId)
                destroy()
            }
        }
    }

    // ── progress ───────────────────────────────────────────────────────────
    Component {
        id: progressComponent
        PluginDialog {
            id: progress
            width: 380
            closePolicy: Popup.NoAutoClose
            iconName: "hourglass_empty"
            property real fraction: -1
            property string text: ""
            property bool cancelRequested: false
            function updateProgress(f, t) { fraction = f; if (t !== "") text = t }
            contentItem: ColumnLayout {
                spacing: 8
                ProgressBar {
                    Layout.fillWidth: true
                    indeterminate: progress.fraction < 0
                    value: progress.fraction < 0 ? 0 : progress.fraction
                }
                Label { text: progress.text; color: Theme.text2; wrapMode: Text.Wrap; Layout.fillWidth: true; visible: text !== "" }
            }
            footer: DialogButtonRow {
                Button {
                    visible: progress.params.cancellable !== false
                    enabled: !progress.cancelRequested
                    text: progress.cancelRequested ? qsTr("Cancelling…") : qsTr("Cancel")
                    onClicked: { progress.cancelRequested = true; PluginUiController.cancelProgress(progress.requestId) }
                }
            }
            // Ended by the plugin (requestClosed), never by the user.
        }
    }

    // ── contact picker ─────────────────────────────────────────────────────
    Component {
        id: contactComponent
        ContactPickerDialog {
            id: contactPicker
            property int requestId: 0
            property var params: ({})
            property bool answered: false
            parent: Overlay.overlay
            headingText: params.title || qsTr("Choose Contact")
            reload: function () { return DesktopAppController.contactList() }
            function dismiss() { answered = true; close() }
            onPicked: (contact) => {
                answered = true
                PluginUiController.respond(requestId, params.multiple ? [contact.id] : contact.id)
            }
            onClosed: {
                if (!answered) PluginUiController.cancelRequest(requestId)
                host._forget(requestId)
                destroy()
            }
        }
    }

    // ── recipient picker (email.chooseRecipients) ──────────────────────────
    // The report recipient panel over an AudienceReviewSession. The answer is
    // only true or null; the host reads the selection from the session.
    Component {
        id: recipientComponent
        PluginDialog {
            id: recipients
            width: Math.min(560, host.width - 40)
            iconName: "group"
            title: params.title || qsTr("Choose Recipients")
            property string error: ""
            contentItem: ScrollView {
                id: recipientScroll
                implicitHeight: Math.min(recipientColumn.implicitHeight, host.height * 0.7)
                contentWidth: availableWidth
                clip: true
                ColumnLayout {
                    id: recipientColumn
                    width: recipientScroll.availableWidth
                    spacing: 8
                    RecipientSelectionPane {
                        Layout.fillWidth: true
                        compact: true
                        recipientModel: recipients.params.session ? recipients.params.session.recipientModel : null
                        audienceController: recipients.params.session || null
                    }
                    Label { visible: recipients.error !== ""; text: recipients.error; color: Theme.red; wrapMode: Text.Wrap; Layout.fillWidth: true }
                }
            }
            footer: DialogButtonRow {
                Button { text: qsTr("Cancel"); onClicked: recipients.answer(null) }
                Button {
                    text: recipients.params.okText || qsTr("OK")
                    primary: true
                    onClicked: {
                        recipients.error = recipients.params.session ? recipients.params.session.problem() : ""
                        if (recipients.error === "") recipients.answer(true)
                    }
                }
            }
        }
    }

    // ── meeting review ─────────────────────────────────────────────────────
    Component {
        id: meetingComponent
        PluginDialog {
            id: meeting
            width: Math.min(560, host.width - 40)
            iconName: "event"
            title: qsTr("Meeting from %1").arg(params.pluginName || "")
            contentItem: ColumnLayout {
                spacing: 6
                Label { text: qsTr("Review the meeting, then create a draft in %1.").arg(meeting.params.backend || ""); color: Theme.text2; wrapMode: Text.Wrap; Layout.fillWidth: true }
                Label { text: meeting.params.subject || ""; color: Theme.text; font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold; wrapMode: Text.Wrap; Layout.fillWidth: true }
                Label { text: qsTr("Starts: %1").arg(meeting.params.start || ""); color: Theme.text }
                Label { text: qsTr("Ends: %1").arg(meeting.params.end || ""); color: Theme.text }
                Label { visible: !!meeting.params.location; text: qsTr("Location: %1").arg(meeting.params.location || ""); color: Theme.text }
                Label { visible: (meeting.params.attendees || []).length > 0; text: qsTr("Attendees: %1").arg((meeting.params.attendees || []).join(", ")); color: Theme.text; wrapMode: Text.Wrap; Layout.fillWidth: true }
                Text { visible: !!meeting.params.html; textFormat: Text.RichText; wrapMode: Text.Wrap; color: Theme.text; text: meeting.params.html || ""; Layout.fillWidth: true }
            }
            footer: DialogButtonRow {
                Button { text: qsTr("Cancel"); onClicked: meeting.answer(false) }
                Button { text: qsTr("Create Meeting Draft"); primary: true; onClicked: meeting.answer(true) }
            }
        }
    }

    component DialogButtonRow: Item {
        default property alias buttons: row.data
        implicitHeight: 54
        RowLayout {
            id: row
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 10
            Item { Layout.fillWidth: true }
        }
    }

    // ── toast (ui.notify, plugin errors) ───────────────────────────────────
    Rectangle {
        id: toast
        property string kind: "info"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: Math.min(toastText.implicitWidth + 32, host.width - 48)
        height: toastText.implicitHeight + 20
        radius: Theme.radius
        visible: opacity > 0
        opacity: 0
        z: 1000
        color: kind === "error" ? Theme.red : kind === "warning" ? Theme.amber : kind === "success" ? Theme.green : Theme.accentStrong
        Behavior on opacity { NumberAnimation { duration: 150 } }
        function show(text, k, seconds) {
            toastText.text = text
            kind = k || "info"
            opacity = 1
            hide.interval = Math.max(1, seconds || 4) * 1000
            hide.restart()
        }
        Label {
            id: toastText
            anchors.centerIn: parent
            width: Math.min(implicitWidth, host.width - 80)
            wrapMode: Text.Wrap
            color: "#ffffff"
        }
        Timer { id: hide; onTriggered: toast.opacity = 0 }
        TapHandler { onTapped: toast.opacity = 0 }
    }
}

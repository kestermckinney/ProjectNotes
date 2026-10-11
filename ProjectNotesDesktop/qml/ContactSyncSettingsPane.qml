// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

ColumnLayout {
    id: pane
    property var syncModel: null
    readonly property var config: syncModel ? syncModel.configuration : ({})
    readonly property bool supported: config.provider && config.provider !== "none"
    spacing: 10
    component ContactCheck: CheckBox {
        id: control
        indicator: Rectangle {
            width: 18
            height: 18
            radius: 3
            y: (control.height - height) / 2
            color: control.checked ? Theme.accent : Theme.surface
            border.color: control.checked ? Theme.accent : Theme.border
            Text {
                anchors.centerIn: parent
                text: "✓"
                color: "white"
                visible: control.checked
            }
        }
        contentItem: Text {
            text: control.text
            color: Theme.text
            font.pixelSize: Theme.fontBody
            leftPadding: 26
            verticalAlignment: Text.AlignVCenter
        }
    }
    function option(key, value) {
        if (syncModel)
            syncModel.setOption(key, value);
    }
    function selectBook(id, checked) {
        let list = Array.from(config.sourceBooks || []);
        if (checked && list.indexOf(id) < 0)
            list.push(id);
        if (!checked)
            list = list.filter(value => value !== id);
        option("sourceBooks", list);
    }
    Text {
        text: qsTr("External contact values replace mapped fields. Possible duplicates wait for review. Deletions are not propagated.")
        color: Theme.text2
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    Text {
        visible: !pane.supported
        text: qsTr("Default Application does not provide contact synchronization. Choose an application under Email, Contacts, and Calendar to synchronize contacts.")
        color: Theme.text2
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    ContactCheck {
        text: qsTr("Enable synchronization")
        enabled: pane.supported
        checked: !!pane.config.enabled
        onClicked: pane.option("enabled", checked)
    }
    Text {
        visible: Qt.platform.os === "windows" && pane.config.provider === "outlook"
        text: qsTr("Requires classic Outlook and its active profile. Use Office 365 for new Outlook cloud contacts.")
        color: Theme.text2
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    Text {
        visible: pane.config.provider === "office365"
        text: qsTr("Uses the shared account in Office 365 Integration. Sign in there to grant contact access for synchronization and full export.")
        color: Theme.text2
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    Text {
        visible: pane.config.provider === "mac"
        text: qsTr("Uses the Contacts app on this Mac directly; there is no separate account or password to enter. The first sync asks for access in System Settings > Privacy & Security > Contacts.")
        color: Theme.text2
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    ColumnLayout {
        visible: pane.config.provider === "icloud"
        Layout.fillWidth: true
        FormField {
            label: qsTr("Apple Account")
            text: pane.config.account || ""
            onEditingFinished: pane.option("account", text)
        }
        FormField {
            label: qsTr("iCloud Mail address (if different)")
            placeholder: qsTr("Uses the Apple Account above when blank")
            text: pane.config.mailAccount || ""
            onEditingFinished: pane.option("mailAccount", text)
        }
        RowLayout {
            TextField {
                id: password
                placeholderText: qsTr("App-specific password (blank keeps stored password)")
                echoMode: TextInput.Password
                Layout.fillWidth: true
                color: Theme.text
                background: Rectangle { color: Theme.surface; border.color: Theme.border; radius: Theme.radiusSm }
                SpellCheckField { target: password; spellCheckEnabled: false }
            }
            Button {
                text: qsTr("Save password")
                onClicked: {
                    pane.syncModel.savePassword(password.text);
                    password.clear();
                }
            }
        }
    }
    RowLayout {
        visible: pane.config.provider === "thunderbird"
        Button {
            objectName: "thunderbirdBridgeSetupButton"
            text: qsTr("Set up Thunderbird bridge")
            enabled: pane.syncModel && !pane.syncModel.thunderbirdSetupBusy
            onClicked: thunderbirdSetupConfirm.open()
        }
        Text {
            text: qsTr("Thunderbird must be running with the Project Notes Contacts add-on.")
            color: Theme.text2
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
    ThemedMessageDialog {
        id: thunderbirdSetupConfirm
        objectName: "thunderbirdBridgeSetupConfirm"
        title: qsTr("Set up Thunderbird bridge")
        width: 520
        iconName: "info"
        iconColor: Theme.accent
        confirmText: qsTr("Set up bridge")
        message: qsTr("This registers the contact bridge helper. You will then install the Project Notes Contacts add-on in Thunderbird; the next screen gives the file path and steps.")
                 + (pane.syncModel && pane.syncModel.thunderbirdFlatpakInstalled
                    ? qsTr("\n\nFlatpak Thunderbird was detected. Setup also enables permission for it to launch programs on this computer so it can run the bridge helper. This permission applies to Thunderbird as a whole.") : "")
        onConfirmed: if (pane.syncModel) pane.syncModel.installThunderbirdHost()
    }
    ThemedMessageDialog {
        id: thunderbirdSetupResult
        objectName: "thunderbirdBridgeSetupResult"
        title: qsTr("Thunderbird bridge setup")
        width: 600
        selectableMessage: true
    }
    Connections {
        target: pane.syncModel
        function onFullExportFinished(message, success) {
            fullExportResult.message = message;
            fullExportResult.iconName = success ? "info" : "error";
            fullExportResult.iconColor = success ? Theme.accent : Theme.red;
            fullExportResult.open();
        }
        function onExportReviewReady() { exportReviewDialog.open(); }
        function onThunderbirdSetupFinished(message, success) {
            thunderbirdSetupResult.message = message;
            thunderbirdSetupResult.iconName = success ? "info" : "error";
            thunderbirdSetupResult.iconColor = success ? Theme.accent : Theme.red;
            thunderbirdSetupResult.open();
        }
    }
    ThemedMessageDialog {
        id: fullExportResult
        objectName: "fullContactExportResult"
        title: qsTr("Contact export result")
        width: 520
        selectableMessage: true
    }
    RowLayout {
        ContactCheck {
            text: qsTr("Sync at startup")
            checked: !!pane.config.startup
            onClicked: pane.option("startup", checked)
        }
        Text {
            text: qsTr("Interval (minutes)")
            color: Theme.text
        }
        SpinBox {
            id: interval
            from: 1
            to: 1440
            value: pane.config.interval || 5
            editable: true
            contentItem: TextField {
                id: intervalEditor
                text: interval.textFromValue(interval.value, interval.locale)
                validator: interval.validator
                inputMethodHints: Qt.ImhDigitsOnly
                horizontalAlignment: Text.AlignHCenter
                color: Theme.text
                background: Rectangle { color: Theme.surface; border.color: Theme.border }
                onEditingFinished: {
                    interval.value = interval.valueFromText(text, interval.locale)
                    pane.option("interval", interval.value)
                }
                SpellCheckField { target: intervalEditor; spellCheckEnabled: false }
            }
            onValueModified: pane.option("interval", value)
        }
        Button {
            text: qsTr("Test Connection")
            enabled: pane.supported && pane.syncModel && !pane.syncModel.busy
            onClicked: pane.syncModel.testConnection()
        }
    }
    Text {
        text: qsTr("Import address books (none selected means all)")
        color: Theme.text
        visible: bookRepeater.count > 0
    }
    Repeater {
        id: bookRepeater
        model: pane.syncModel ? pane.syncModel.books : []
        ContactCheck {
            required property var modelData
            text: modelData.name || modelData.id
            checked: (pane.config.sourceBooks || []).indexOf(modelData.id) >= 0
            onClicked: pane.selectBook(modelData.id, checked)
        }
    }
    RowLayout {
        Button {
            objectName: "fullContactExportButton"
            text: qsTr("Full Contact Export")
            enabled: pane.supported && pane.syncModel && !pane.syncModel.busy
            onClicked: fullExportWarning.open()
        }
        Text {
            text: qsTr("Checks for existing contacts before creating, updating, or skipping each active Project Notes contact.")
            color: Theme.text2
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
    ThemedMessageDialog {
        id: fullExportWarning
        objectName: "fullContactExportWarning"
        title: qsTr("Full Contact Export")
        message: qsTr("Project Notes will check the selected contact system first. You will review possible matches and choose whether to update, skip, or create a new contact.")
        iconName: "warning"
        iconColor: Theme.red
        confirmText: qsTr("Export Contacts")
        destructive: true
        onConfirmed: if (pane.syncModel) pane.syncModel.fullExport()
    }
    ContactExportReviewDialog {
        id: exportReviewDialog
        objectName: "contactExportReviewDialog"
        syncModel: pane.syncModel
    }
    RowLayout {
        Button {
            text: qsTr("Sync Now")
            primary: true
            enabled: pane.supported && pane.syncModel && !pane.syncModel.busy
            onClicked: pane.syncModel.syncNow()
        }
        Button {
            text: qsTr("Cancel")
            visible: pane.syncModel && pane.syncModel.busy
            onClicked: pane.syncModel.cancelSync()
        }
        Text {
            text: qsTr("Last success: %1").arg(pane.syncModel && pane.syncModel.lastSuccess ? pane.syncModel.lastSuccess : qsTr("Never"))
            color: Theme.text2
        }
    }
    Text {
        text: pane.syncModel ? pane.syncModel.status : ""
        color: Theme.text2
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    Button {
        objectName: "reviewContactConflictsButton"
        text: qsTr("Review Contact Conflicts (%1)").arg(pane.syncModel ? pane.syncModel.reviews.length : 0)
        enabled: pane.syncModel && pane.syncModel.reviews.length > 0
        onClicked: conflictDialog.open()
        Rectangle {
            objectName: "reviewContactsDot"
            width: 8; height: 8; radius: 4
            color: Theme.red
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 3
            visible: pane.syncModel && pane.syncModel.reviews.length > 0
        }
    }
    ContactConflictReviewDialog {
        id: conflictDialog
        objectName: "contactConflictReviewDialog"
        syncModel: pane.syncModel
    }
}

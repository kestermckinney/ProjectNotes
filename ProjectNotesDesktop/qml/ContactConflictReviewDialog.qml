// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

Dialog {
    id: dialog
    property var syncModel: null
    function sourceName(scope) {
        switch ((scope || "").split("/")[0]) {
        case "outlook": return qsTr("Outlook");
        case "thunderbird": return qsTr("Thunderbird");
        case "office365": return qsTr("Office 365");
        case "icloud": return qsTr("iCloud");
        default: return qsTr("Contact source");
        }
    }
    component ContactDetail: RowLayout {
        property string caption: ""
        property string value: ""
        Layout.fillWidth: true
        spacing: 8
        Text {
            text: caption
            color: Theme.text2
            Layout.preferredWidth: 100
            Layout.alignment: Qt.AlignTop
        }
        Text {
            text: value || qsTr("Not provided")
            color: Theme.text
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
    modal: true
    dim: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    scale: Theme.uiScale
    width: Math.min(940, parent ? (parent.width - 36) / scale : 940)
    height: Math.min(720, parent ? (parent.height - 36) / scale : 720)
    padding: 0
    readonly property bool clientStage: syncModel && syncModel.reviews.some(entry => entry.stage === "client")
    title: clientStage ? qsTr("Step 1: Resolve Duplicate Clients") : qsTr("Step 2: Resolve Duplicate Contacts")
    header: null
    ClickShield { host: dialog }
    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            Label {
                text: dialog.title
                color: Theme.text
                font.pixelSize: Theme.fontXl
                font.bold: true
                Layout.fillWidth: true
            }
            DialogCloseButton { popup: dialog }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 14
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ColumnLayout {
                width: scroll.availableWidth
                spacing: 12
                Label {
                    text: qsTr("No contact conflicts to review.")
                    visible: reviewRepeater.count === 0
                    color: Theme.text2
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                Label {
                    text: dialog.clientStage ? qsTr("Compare each imported client with the possible Project Notes client. Choose either client, or keep both. Contact duplicates will be checked after all client choices are saved.") : qsTr("Client matches are resolved. Choose which contact details to keep.")
                    visible: reviewRepeater.count > 0
                    color: Theme.text2
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                Repeater {
                    id: reviewRepeater
                    objectName: "contactConflictRepeater"
                    model: dialog.syncModel ? dialog.syncModel.reviews : []
                    ColumnLayout {
                        id: review
                        required property var modelData
                        readonly property bool clientReview: modelData.stage === "client"
                        readonly property var contact: modelData.contact || ({})
                        readonly property var rightContact: hasPerson ? modelData.people[personChoice.currentIndex].contact || ({}) : ({})
                        readonly property bool hasClient: clientChoice.currentIndex >= 0 && clientChoice.currentIndex < (modelData.clients || []).length
                        readonly property var rightClient: hasClient ? modelData.clients[clientChoice.currentIndex] : ({})
                        readonly property bool exportReview: modelData.key.startsWith("export/")
                        readonly property bool hasPerson: personChoice.currentIndex >= 0 && personChoice.currentIndex < (modelData.people || []).length
                        Layout.fillWidth: true
                        ColumnLayout {
                            visible: review.clientReview
                            Layout.fillWidth: true
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 24
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    Layout.alignment: Qt.AlignTop
                                    spacing: 10
                                    Label { text: dialog.sourceName(review.modelData.scope); color: Theme.text; font.bold: true; font.pixelSize: Theme.fontLg }
                                    ContactDetail { caption: qsTr("Client:"); value: review.modelData.company || "" }
                                    Text {
                                        text: review.modelData.reason || ""
                                        color: Theme.text2
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                    Button {
                                        objectName: "useLeftClientButton"
                                        text: qsTr("Use This Client")
                                        enabled: review.hasClient && dialog.syncModel && !dialog.syncModel.busy
                                        onClicked: dialog.syncModel.resolve(review.modelData.key, "clientUseLeft", "", review.rightClient.id, "")
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    Layout.alignment: Qt.AlignTop
                                    spacing: 10
                                    Label { text: qsTr("Project Notes"); color: Theme.text; font.bold: true; font.pixelSize: Theme.fontLg }
                                    StyledComboBox {
                                        id: clientChoice
                                        objectName: "duplicateClientChoice"
                                        model: review.modelData.clients || []
                                        textRole: "label"
                                        Layout.fillWidth: true
                                        visible: count > 1
                                    }
                                    ContactDetail { caption: qsTr("Client:"); value: review.rightClient.label || "" }
                                    Text {
                                        text: review.hasClient ? review.rightClient.reason : ""
                                        color: Theme.text2
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                    Button {
                                        objectName: "useRightClientButton"
                                        text: qsTr("Use This Client")
                                        enabled: review.hasClient && dialog.syncModel && !dialog.syncModel.busy
                                        onClicked: dialog.syncModel.resolve(review.modelData.key, "clientUseRight", "", review.rightClient.id, "")
                                    }
                                }
                            }
                            Button {
                                objectName: "useBothClientsButton"
                                text: qsTr("Use Both")
                                enabled: review.hasClient && dialog.syncModel && !dialog.syncModel.busy
                                Layout.alignment: Qt.AlignRight
                                onClicked: dialog.syncModel.resolve(review.modelData.key, "clientUseBoth", "", "", "")
                            }
                        }
                        RowLayout {
                            visible: !review.clientReview
                            Layout.fillWidth: true
                            spacing: 24
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.alignment: Qt.AlignTop
                                spacing: 10
                                Label {
                                    text: review.modelData.key.startsWith("export/") ? qsTr("Project Notes") : dialog.sourceName(review.modelData.scope)
                                    color: Theme.text
                                    font.bold: true
                                    font.pixelSize: Theme.fontLg
                                    Layout.fillWidth: true
                                }
                                ContactDetail {
                                    caption: qsTr("Name:")
                                    value: review.contact.name || ""
                                }
                                ContactDetail {
                                    caption: qsTr("Company:")
                                    value: review.contact.company || ""
                                }
                                ContactDetail {
                                    caption: qsTr("Email:")
                                    value: (review.contact.emails || []).length ? review.contact.emails.join(", ") : (review.contact.fields || {}).email || ""
                                }
                                ContactDetail {
                                    caption: qsTr("Office phone:")
                                    value: (review.contact.fields || {}).office_phone || ""
                                }
                                ContactDetail {
                                    caption: qsTr("Cell phone:")
                                    value: (review.contact.fields || {}).cell_phone || ""
                                }
                                ContactDetail {
                                    caption: qsTr("Other phones:")
                                    value: (review.contact.phones || []).filter(p => p !== (review.contact.fields || {}).office_phone && p !== (review.contact.fields || {}).cell_phone).join(", ")
                                }
                                Text {
                                    text: review.modelData.reason
                                    color: Theme.text2
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                                Button {
                                    objectName: "useLeftContactButton"
                                    text: qsTr("Use This One")
                                    visible: !review.exportReview && !review.clientReview
                                    enabled: dialog.syncModel && !dialog.syncModel.busy
                                    onClicked: dialog.syncModel.resolve(review.modelData.key, "useLeft", review.hasPerson ? review.modelData.people[personChoice.currentIndex].id : "", "", "")
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.alignment: Qt.AlignTop
                                spacing: 10
                                Label {
                                    text: review.exportReview ? qsTr("Export resolution") : qsTr("Project Notes")
                                    color: Theme.text
                                    font.bold: true
                                    font.pixelSize: Theme.fontLg
                                    Layout.fillWidth: true
                                }
                                StyledComboBox {
                                    id: personChoice
                                    model: review.modelData.people || []
                                    textRole: "label"
                                    Layout.fillWidth: true
                                    visible: count > 1
                                }
                                ContactDetail { caption: qsTr("Name:"); value: review.rightContact.name || "" }
                                ContactDetail { caption: qsTr("Company:"); value: review.rightContact.company || "" }
                                ContactDetail { caption: qsTr("Email:"); value: (review.rightContact.fields || {}).email || "" }
                                ContactDetail {
                                    caption: qsTr("Office phone:")
                                    value: (review.rightContact.fields || {}).office_phone || ""
                                }
                                ContactDetail {
                                    caption: qsTr("Cell phone:")
                                    value: (review.rightContact.fields || {}).cell_phone || ""
                                }
                                ContactDetail {
                                    caption: qsTr("Other phones:")
                                    value: (review.rightContact.phones || []).filter(p => p !== (review.rightContact.fields || {}).office_phone && p !== (review.rightContact.fields || {}).cell_phone).join(", ")
                                }
                                Text {
                                    text: review.hasPerson ? review.modelData.people[personChoice.currentIndex].reason : ""
                                    color: Theme.text2
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                                Button {
                                    objectName: "useRightContactButton"
                                    text: qsTr("Use This One")
                                    visible: !review.exportReview && !review.clientReview
                                    enabled: review.hasPerson && dialog.syncModel && !dialog.syncModel.busy
                                    onClicked: dialog.syncModel.resolve(review.modelData.key, "ignore", "", "", "")
                                }
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    visible: review.exportReview
                                    Button {
                                        text: qsTr("Ignore")
                                        onClicked: dialog.syncModel.resolve(review.modelData.key, "ignore", "", "", "")
                                    }
                                    Button {
                                        text: qsTr("I checked the destination; allow retry")
                                        onClicked: dialog.syncModel.resolve(review.modelData.key, "retry", "", "", "")
                                    }
                                }
                            }
                        }
                        Button {
                            objectName: "useBothContactsButton"
                            text: qsTr("Use Both")
                            visible: !review.exportReview && !review.clientReview
                            enabled: review.hasPerson && dialog.syncModel && !dialog.syncModel.busy
                            Layout.alignment: Qt.AlignRight
                            onClicked: dialog.syncModel.resolve(review.modelData.key, "useBoth", "", "", "")
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Theme.border
                        }
                    }
                }
            }
        }
        Label {
            text: dialog.syncModel ? dialog.syncModel.status : ""
            color: Theme.text2
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            Layout.margins: 14
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Close"); onClicked: Qt.callLater(dialog.close) }
        }
    }
}

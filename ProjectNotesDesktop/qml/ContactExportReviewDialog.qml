// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

Dialog {
    id: dialog
    property var syncModel: null
    function sourceName() {
        const provider = syncModel ? syncModel.configuration.provider : "";
        switch (provider) {
        case "outlook": return qsTr("Outlook");
        case "thunderbird": return qsTr("Thunderbird");
        case "office365": return qsTr("Office 365");
        case "icloud": return qsTr("iCloud");
        default: return qsTr("Existing Contact");
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
    width: Math.min(900, parent ? (parent.width - 36) / scale : 900)
    height: Math.min(680, parent ? (parent.height - 36) / scale : 680)
    padding: 0
    header: null
    closePolicy: Popup.NoAutoClose
    ClickShield { host: dialog }
    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }
    Connections {
        target: dialog.syncModel
        function onChanged() {
            if (dialog.opened && dialog.syncModel && dialog.syncModel.exportReviews.length === 0)
                dialog.close();
        }
    }
    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            Label { text: qsTr("Review Existing Contacts"); color: Theme.text; font.pixelSize: Theme.fontXl; font.bold: true; Layout.fillWidth: true }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        Label {
            text: qsTr("Compare each Project Notes contact with the possible existing contact. Choose which contact to use, or use both to create a separate contact.")
            color: Theme.text2; wrapMode: Text.Wrap; Layout.fillWidth: true; Layout.margins: 14
        }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; padding: 14; clip: true; contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ColumnLayout {
                width: parent.width
                Repeater {
                    id: exportReviewRepeater
                    objectName: "contactExportReviewRepeater"
                    model: dialog.syncModel ? dialog.syncModel.exportReviews : []
                    ColumnLayout {
                        id: review
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 10
                        readonly property var selected: candidateChoice.currentIndex >= 0 ? modelData.candidates[candidateChoice.currentIndex] : null
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 24
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.alignment: Qt.AlignTop
                                spacing: 10
                                Label { text: qsTr("Project Notes"); color: Theme.text; font.bold: true; font.pixelSize: Theme.fontLg }
                                ContactDetail { caption: qsTr("Name:"); value: review.modelData.contact.name || "" }
                                ContactDetail { caption: qsTr("Company:"); value: review.modelData.contact.company || "" }
                                ContactDetail {
                                    caption: qsTr("Email:")
                                    value: (review.modelData.contact.emails || []).length ? review.modelData.contact.emails.join(", ") : (review.modelData.contact.fields || {}).email || ""
                                }
                                ContactDetail { caption: qsTr("Office phone:"); value: (review.modelData.contact.fields || {}).office_phone || "" }
                                ContactDetail { caption: qsTr("Cell phone:"); value: (review.modelData.contact.fields || {}).cell_phone || "" }
                                ContactDetail { caption: qsTr("Role:"); value: (review.modelData.contact.fields || {}).role || "" }
                                Button {
                                    objectName: "useProjectNotesExportContactButton"
                                    text: qsTr("Use This One")
                                    primary: true
                                    enabled: review.selected && review.selected.writable
                                    onClicked: dialog.syncModel.resolveExport(review.modelData.personId, "update", review.selected.id)
                                }
                                Text {
                                    visible: review.selected && !review.selected.writable
                                    text: qsTr("This address book is read-only, so the existing contact cannot be updated.")
                                    color: Theme.text2; wrapMode: Text.Wrap; Layout.fillWidth: true
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.alignment: Qt.AlignTop
                                spacing: 10
                                Label { text: dialog.sourceName(); color: Theme.text; font.bold: true; font.pixelSize: Theme.fontLg }
                                StyledComboBox {
                                    id: candidateChoice
                                    objectName: "exportContactCandidateChoice"
                                    model: review.modelData.candidates || []
                                    textRole: "label"
                                    Layout.fillWidth: true
                                    visible: count > 1
                                }
                                ContactDetail { caption: qsTr("Name:"); value: review.selected ? review.selected.contact.name || "" : "" }
                                ContactDetail { caption: qsTr("Company:"); value: review.selected ? review.selected.contact.company || "" : "" }
                                ContactDetail {
                                    caption: qsTr("Email:")
                                    value: review.selected ? ((review.selected.contact.emails || []).length ? review.selected.contact.emails.join(", ") : (review.selected.contact.fields || {}).email || "") : ""
                                }
                                ContactDetail { caption: qsTr("Office phone:"); value: review.selected ? (review.selected.contact.fields || {}).office_phone || "" : "" }
                                ContactDetail { caption: qsTr("Cell phone:"); value: review.selected ? (review.selected.contact.fields || {}).cell_phone || "" : "" }
                                ContactDetail { caption: qsTr("Role:"); value: review.selected ? (review.selected.contact.fields || {}).role || "" : "" }
                                Text {
                                    text: review.selected ? qsTr("%1 · Address book: %2%3").arg(review.selected.reason).arg(review.selected.bookName).arg(review.selected.writable ? "" : qsTr(" (read-only)")) : ""
                                    color: Theme.text2; wrapMode: Text.Wrap; Layout.fillWidth: true
                                }
                                Button {
                                    objectName: "useExistingExportContactButton"
                                    text: qsTr("Use This One")
                                    enabled: review.selected
                                    onClicked: dialog.syncModel.resolveExport(review.modelData.personId, "skip", review.selected.id)
                                }
                            }
                        }
                        Button {
                            objectName: "useBothExportContactsButton"
                            text: qsTr("Use Both")
                            enabled: review.selected
                            Layout.alignment: Qt.AlignRight
                            onClicked: dialog.syncModel.resolveExport(review.modelData.personId, "create", "")
                        }
                        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: 8; Layout.bottomMargin: 8 }
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 14
            Label { text: dialog.syncModel ? dialog.syncModel.status : ""; color: Theme.text2; Layout.fillWidth: true; wrapMode: Text.Wrap }
            Button { text: qsTr("Cancel Export"); onClicked: { dialog.syncModel.cancelSync(); dialog.close(); } }
        }
    }
}

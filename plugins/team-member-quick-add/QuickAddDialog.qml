// Team Member Quick Add dialog. Built from the application's own controls
// (ProjectNotes.PluginUI); answers through pluginDialog.
import QtQuick
import QtQuick.Layouts
import ProjectNotes.PluginUI

ColumnLayout {
    spacing: 8

    FormField { id: name; Layout.fillWidth: true; label: qsTr("Name *") }
    FormField { id: email; Layout.fillWidth: true; label: qsTr("Email") }
    RowLayout {
        Layout.fillWidth: true
        FormField { id: office; Layout.fillWidth: true; label: qsTr("Office phone") }
        FormField { id: mobile; Layout.fillWidth: true; label: qsTr("Mobile phone") }
    }
    ComboField {
        id: company
        Layout.fillWidth: true
        label: qsTr("Company")
        options: pluginDialog.data.companies
        includeNone: true
        searchable: true
        onActivated: (v) => value = v
    }
    FormField { id: newCompany; Layout.fillWidth: true; label: qsTr("Or a new company") }
    FormField { id: role; Layout.fillWidth: true; label: qsTr("Role") }

    Item { Layout.fillHeight: true }

    RowLayout {
        Layout.fillWidth: true
        Item { Layout.fillWidth: true }
        Button { text: qsTr("Cancel"); onClicked: pluginDialog.reject() }
        Button {
            text: qsTr("Add")
            primary: true
            enabled: name.text.trim() !== ""
            onClicked: pluginDialog.accept({
                name: name.text, email: email.text, officePhone: office.text, mobilePhone: mobile.text,
                company: company.value === company.noneLabel ? "" : company.value,
                newCompany: newCompany.text, role: role.text
            })
        }
    }
}

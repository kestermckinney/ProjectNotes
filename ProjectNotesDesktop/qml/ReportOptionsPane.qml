// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Pure view state for the three report dialogs. The owning controller supplies
// options and reacts to optionChanged; no report, audience, or backend logic
// belongs in this component.
ColumnLayout {
    id: pane
    spacing: 10
    property string workflow: "tracker-items-report"
    property bool displayPdf: false
    property bool internalReport: false
    property bool retainHtml: false
    property string emailMode: "inline-html"
    property bool trackerItems: true
    property bool actionItems: false
    property bool statusNew: true
    property bool statusAssigned: true
    property bool statusResolved: false
    property bool statusDefered: false
    property bool statusCancelled: false
    signal optionChanged()

    Label { text: qsTr("Report options"); color: Theme.text; font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold }
    CheckBox { text: qsTr("Display report when complete"); checked: pane.displayPdf; onToggled: { pane.displayPdf = checked; pane.optionChanged() } }
    CheckBox { text: qsTr("Generate internal report"); checked: pane.internalReport; onToggled: { pane.internalReport = checked; pane.optionChanged() } }
    CheckBox { text: qsTr("Generate HTML report (temporary)"); checked: pane.retainHtml; onToggled: { pane.retainHtml = checked; pane.optionChanged() } }

    ColumnLayout {
        visible: pane.workflow === "tracker-items-report"
        Layout.fillWidth: true
        spacing: 4
        Label { text: qsTr("Include"); color: Theme.text2; font.weight: Font.DemiBold }
        CheckBox { text: qsTr("Tracker items"); checked: pane.trackerItems; onToggled: { pane.trackerItems = checked; pane.optionChanged() } }
        CheckBox { text: qsTr("Action items"); checked: pane.actionItems; onToggled: { pane.actionItems = checked; pane.optionChanged() } }
        Label { text: qsTr("Status"); color: Theme.text2; font.weight: Font.DemiBold }
        CheckBox { text: qsTr("New"); checked: pane.statusNew; onToggled: { pane.statusNew = checked; pane.optionChanged() } }
        CheckBox { text: qsTr("Assigned"); checked: pane.statusAssigned; onToggled: { pane.statusAssigned = checked; pane.optionChanged() } }
        CheckBox { text: qsTr("Resolved"); checked: pane.statusResolved; onToggled: { pane.statusResolved = checked; pane.optionChanged() } }
        CheckBox { text: qsTr("Deferred"); checked: pane.statusDefered; onToggled: { pane.statusDefered = checked; pane.optionChanged() } }
        CheckBox { text: qsTr("Cancelled"); checked: pane.statusCancelled; onToggled: { pane.statusCancelled = checked; pane.optionChanged() } }
    }

    Label { text: qsTr("Email report as"); color: Theme.text2; font.weight: Font.DemiBold }
    ButtonGroup { id: emailModeGroup }
    RadioButton { text: qsTr("Inline HTML"); ButtonGroup.group: emailModeGroup; checked: pane.emailMode === "inline-html"; onToggled: if (checked) { pane.emailMode = "inline-html"; pane.optionChanged() } }
    RadioButton { text: qsTr("HTML attachment"); ButtonGroup.group: emailModeGroup; checked: pane.emailMode === "html-attachment"; onToggled: if (checked) { pane.emailMode = "html-attachment"; pane.optionChanged() } }
    RadioButton { text: qsTr("PDF attachment"); ButtonGroup.group: emailModeGroup; checked: pane.emailMode === "pdf-attachment"; onToggled: if (checked) { pane.emailMode = "pdf-attachment"; pane.optionChanged() } }
    RadioButton { text: qsTr("Do not email"); ButtonGroup.group: emailModeGroup; checked: pane.emailMode === "none"; onToggled: if (checked) { pane.emailMode = "none"; pane.optionChanged() } }
}

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Unified "Move To…" picker for a tracker item: type-to-search combos for the
// destination project and, scoped to whichever project is currently picked, a
// meeting note to link it to (or "No meeting"). Defaults to the item's current
// project and meeting. Picking a different project resets the meeting choice
// to "No meeting" — meetings are project-scoped, so the old selection can't
// carry over — but the user may then pick one of the new project's meetings
// before confirming. Nothing is written until Move is pressed; the dialog
// performs the move itself — renumber/team-add consequences of a project
// change are shown inline rather than in a separate review step.
Dialog {
    id: dlg
    anchors.centerIn: parent
    width: 360; height: content.implicitHeight + topPadding + bottomPadding
    modal: true; padding: 0
    scale: Theme.uiScale   // match the zoomed workspace (centered origin)
    background: Rectangle { radius: Theme.radius; color: Theme.raise; border.color: Theme.border }

    // Clicking away dismisses the dialog and nothing else — see ClickShield.qml.
    ClickShield { host: dlg }

    property string _itemId: ""
    property string _originalProjectId: ""
    property string _selectedProjectId: ""
    property string _selectedNoteId: ""
    property var _projects: []   // [{id,name}] — name = "number  project name"
    property var _meetings: []   // [{id,name}] — name = "date  title", scoped to _selectedProjectId
    // Renumber / team-add preview for the *currently selected* project, versus
    // the item's original one — { valid, willRenumber, oldNumber, newNumber,
    // membersToAdd:[{id,name}] }. Only meaningful (valid) when the selected
    // project differs from the original.
    property var _check: ({})

    function _idForProject(n) { for (var i = 0; i < _projects.length; i++) if (_projects[i].name === n) return _projects[i].id; return "" }
    function _nameForProject(id) { for (var i = 0; i < _projects.length; i++) if (_projects[i].id === id) return _projects[i].name; return "" }
    function _idForMeeting(n) { for (var i = 0; i < _meetings.length; i++) if (_meetings[i].name === n) return _meetings[i].id; return "" }
    function _nameForMeeting(id) { for (var i = 0; i < _meetings.length; i++) if (_meetings[i].id === id) return _meetings[i].name; return "" }

    function _loadMeetings(projectId) {
        var raw = DesktopAppController.notesForProject(projectId)
        var out = []
        for (var i = 0; i < raw.length; i++)
            out.push({ id: raw[i].id, name: ((raw[i].date || "") + "  " + (raw[i].title || "")).trim() })
        dlg._meetings = out
    }

    function openFor(itemId) {
        dlg._itemId = itemId
        DesktopAppController.openTrackerItem(itemId)
        var d = DesktopAppController.getTrackerItemDetailData(0)
        dlg._originalProjectId = (d.project_id || "").toString()
        dlg._selectedProjectId = dlg._originalProjectId
        dlg._selectedNoteId    = (d.note_id || "").toString()
        dlg._check = {}
        dlg._projects = DesktopAppController.projectList()
        projectCombo.value = dlg._nameForProject(dlg._selectedProjectId)
        dlg._loadMeetings(dlg._selectedProjectId)
        meetingCombo.value = dlg._selectedNoteId === "" ? "" : dlg._nameForMeeting(dlg._selectedNoteId)
        open()
    }

    // Selecting a different project: meetings are project-scoped, so the old
    // meeting choice can't carry over — reset to "No meeting" and reload the
    // meeting combo's options for the newly selected project.
    function _pickProject(name) {
        var pid = dlg._idForProject(name)
        if (pid === "" || pid === dlg._selectedProjectId) return
        dlg._selectedProjectId = pid
        dlg._selectedNoteId = ""
        dlg._loadMeetings(pid)
        meetingCombo.value = ""
        dlg._check = pid === dlg._originalProjectId
            ? {} : DesktopAppController.checkTrackerItemMove(dlg._itemId, pid)
    }

    function _confirm() {
        DesktopAppController.moveTrackerItem(dlg._itemId, dlg._selectedProjectId, dlg._selectedNoteId)
        dlg.close()
    }

    contentItem: ColumnLayout {
        id: content
        spacing: 0
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 12
            Text { text: qsTr("Move To…"); color: Theme.text; font.pixelSize: Theme.fontXl; font.weight: Font.Bold; Layout.fillWidth: true }
            MaterialIcon { name: "close"; size: 18; color: Theme.text3; TapHandler { onTapped: dlg.close() } }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 13
            spacing: 10

            ComboField {
                id: projectCombo
                label: qsTr("Project")
                Layout.fillWidth: true
                searchable: true
                options: dlg._projects.map(function(p) { return p.name })
                onActivated: (v) => dlg._pickProject(v)
            }
            ComboField {
                id: meetingCombo
                label: qsTr("Meeting")
                Layout.fillWidth: true
                searchable: true
                includeNone: true
                noneLabel: qsTr("No meeting")
                options: dlg._meetings.map(function(m) { return m.name })
                onActivated: (v) => dlg._selectedNoteId = dlg._idForMeeting(v)
            }

            // Inline renumber / team-add hints — only when the selected
            // project actually differs from the item's current one.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                visible: dlg._check.valid === true
                Text {
                    Layout.fillWidth: true
                    visible: dlg._check.willRenumber === true
                    text: qsTr("Item number will change from %1 to %2 in this project.")
                            .arg(dlg._check.oldNumber || "").arg(dlg._check.newNumber || "")
                    color: Theme.text2; font.pixelSize: Theme.fontXs; wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    visible: (dlg._check.membersToAdd || []).length > 0
                    text: qsTr("Will also be added to the project's team: %1")
                            .arg((dlg._check.membersToAdd || []).map(function(m){ return m.name }).join(", "))
                    color: Theme.text3; font.pixelSize: Theme.fontXs; wrapMode: Text.WordWrap
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 8
            Item { Layout.fillWidth: true }
            Rectangle {
                implicitWidth: 70; implicitHeight: 28; radius: Theme.radiusSm
                color: cancelHover.hovered ? Theme.surface2 : "transparent"
                border.color: Theme.border
                Text { anchors.centerIn: parent; text: qsTr("Cancel"); color: Theme.text2; font.pixelSize: Theme.fontBody }
                HoverHandler { id: cancelHover }
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dlg.close() }
            }
            Rectangle {
                implicitWidth: 78; implicitHeight: 28; radius: Theme.radiusSm
                color: moveHover.hovered ? Theme.accentStrong : Theme.accent
                Text { anchors.centerIn: parent; text: qsTr("Move"); color: "#ffffff"; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold }
                HoverHandler { id: moveHover }
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: dlg._confirm() }
            }
        }
    }
}

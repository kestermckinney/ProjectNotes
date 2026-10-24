// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProjectNotesMobile

Page {
    id: root
    title: qsTr("File / Folder")

    property int    locationRow:        -1
    property string locationId:         ""
    property string initialType:        ""
    property string initialDescription: ""
    property string initialPath:        ""
    property bool   _skipSave:          false
    property bool   _hasChanges:        false
    property bool   isNewRecord:        false

    function _isBlankNew() { return isNewRecord && descField.text.trim() === "" && pathField.text.trim() === "" }
    function _discardNew()  {
        var row = AppController.rowForId(AppController.projectLocationsModel, root.locationId)
        if (row < 0) return
        AppController.deleteProjectLocation(row)
    }

    // Re-resolve locationRow from the stable locationId before every write —
    // Sort/refresh elsewhere in the app can reorder or reset the shared
    // projectLocationsModel proxy while this page is open.
    function _saveNow() {
        if (!root._hasChanges) return true
        var row = AppController.rowForId(AppController.projectLocationsModel, root.locationId)
        if (row < 0) return false   // record no longer exists
        root.locationRow = row
        var locType = typeCombo.selection
        var result = AppController.saveProjectLocation(root.locationRow, locType, descField.text, pathField.text)
        if (result) root._hasChanges = false
        return result
    }

    function _reloadData() {
        var d = AppController.getProjectLocationData(root.locationRow)
        typeCombo.selectText((d.location_type || "").toString(), 0)
        descField.text = (d.location_description || "").toString()
        pathField.text = (d.full_path            || "").toString()
    }

    StackView.onDeactivating: {
        if (!root._skipSave)
            root._saveNow()
    }

    Component.onDestruction: {
        root.forceActiveFocus()
        Qt.inputMethod.hide()
        if (!root._skipSave)
            root._saveNow()
    }

    // ── Toolbar: copy + delete ────────────────────────────────────────────────
    // ── Actions ──────────────────────────────────────────────────────────────
    // Glass pill floating at the top right, over the form as it scrolls.
    NavButton {
        id: navButton
        anchors { left: parent.left; leftMargin: 12; verticalCenter: topBar.verticalCenter }
    }

    GlassBar {
        id: topBar
        z: 2
        anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 12 }
        height: 44
        width: topBarRow.implicitWidth + 2 * contentPadding
        contentPadding: 6
        blurSource: scrollView
        blurRect: Qt.rect(topBar.x - scrollView.x, topBar.y - scrollView.y,
                          topBar.width, topBar.height)

        RowLayout {
            id: topBarRow
            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
            spacing: 0
            BarButton {
                icon.name: "doc.on.doc"
                onClicked: {
                    if (!root._saveNow()) return
                    root._skipSave = true
                    var newRow = AppController.copyProjectLocation(root.locationRow)
                    if (newRow < 0) { root._skipSave = false; return }
                    var d = AppController.getProjectLocationData(newRow)
                    root.StackView.view.replace(Qt.resolvedUrl("ProjectLocationDetailPage.qml"), {
                        locationRow:         newRow,
                        locationId:          (d.id                    || "").toString(),
                        initialType:         (d.location_type        || "").toString(),
                        initialDescription:  (d.location_description || "").toString(),
                        initialPath:         (d.full_path            || "").toString()
                    })
                }
            }

            BarButton {
                icon.name: "trash"
                onClicked: {
                    var row = AppController.rowForId(AppController.projectLocationsModel, root.locationId)
                    if (row >= 0 && AppController.deleteProjectLocation(row)) {
                        root._skipSave = true
                        root.StackView.view.pop()
                    }
                }
            }
        }
    }

    // ── Footer: open ──────────────────────────────────────────────────────────
    // ── Bottom bar ───────────────────────────────────────────────────────────
    // Glass capsule floating over the bottom of the form, like the tab bar.
    GlassBar {
        id: bottomBar
        z: 2
        visible: pathField.text.startsWith("http://") || pathField.text.startsWith("https://")
                 || pathField.text.startsWith("ms-")
                 || typeCombo.selection === "Web Link"
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; bottomMargin: 8; leftMargin: 16; rightMargin: 16 }
        height: 60
        blurSource: scrollView
        blurRect: Qt.rect(bottomBar.x - scrollView.x, bottomBar.y - scrollView.y,
                          bottomBar.width, bottomBar.height)

        RowLayout {
            anchors.centerIn: parent
            BarButton {
                icon.name: "safari"
                text: qsTr("Open")
                display: AbstractButton.TextUnderIcon
                onClicked: {
                    // Save any pending edits first so the row AppController
                    // opens reflects what's on screen, then re-resolve the
                    // row the same way _saveNow() does.
                    if (!root._saveNow()) return
                    var row = AppController.rowForId(AppController.projectLocationsModel, root.locationId)
                    if (row < 0) return
                    root.locationRow = row
                    AppController.openProjectLocation(row)
                }
            }
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0
            // Room for the floating action pill above the first field.
            Item { Layout.preferredHeight: topBar.height + 6 }


            SectionHeader { text: qsTr("Type") }
            FieldRow {
                FormCombo {
                    id: typeCombo
                    options: AppController.fileTypeOptions()
                    Component.onCompleted: selectText(root.initialType, 0)
                    onActivated: root._hasChanges = true
                }
            }

            SectionHeader { text: qsTr("Description") }
            FieldRow {
                FormField {
                    id: descField
                    text: root.initialDescription
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onTextChanged: root._hasChanges = true
                }
            }

            SectionHeader { text: qsTr("Path / URL") }
            FieldRow {
                FormField {
                    id: pathField
                    text: root.initialPath
                    inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText
                    onTextChanged: root._hasChanges = true
                }
            }

            Item { Layout.preferredHeight: 24 }
            // Room for the floating bottom bar below the last field.
            Item { Layout.preferredHeight: bottomBar.visible ? bottomBar.height + 16 : 0 }
        }
    }

    component SectionHeader: Label {
        Layout.fillWidth: true
        Layout.topMargin: 10
        leftPadding: 16
        bottomPadding: 4
        font.pixelSize: Theme.footnote; font.letterSpacing: Theme.tracking(Theme.footnote)
        font.weight: 600
        color: Theme.sectionText
    }
}

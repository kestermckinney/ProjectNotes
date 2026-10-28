// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// The application's standard rich-text editing surface.  Keeping the editor,
// formatting, find/replace, and spelling tools together makes a new rich-text
// field behave consistently with notes by default.
ColumnLayout {
    id: root

    property string value: ""
    property string placeholderText: ""
    property int minimumHeight: 190
    property bool editingEnabled: true
    property var insertFields: []
    property alias editor: textEdit
    property string _lastEmittedValue: ""
    property bool _settingValue: false
    property bool _ready: false

    signal valueEdited(string value)
    signal insertFieldRequested(string path)

    Layout.fillWidth: true
    spacing: 7

    function documentHtml() {
        return TextFormatter.documentHtml(textEdit.textDocument)
    }

    // Do not reparse the document when its own edits update a bound model.
    // Reparsing would reset the cursor and selection after each keystroke.
    onValueChanged: {
        if (!_ready)
            return
        if (value === _lastEmittedValue)
            return
        _settingValue = true
        textEdit.text = value
        _lastEmittedValue = value
        _settingValue = false
    }

    Component.onCompleted: {
        _ready = true
        _settingValue = true
        textEdit.text = value
        _lastEmittedValue = value
        _settingValue = false
    }

    Shortcut {
        sequences: [ StandardKey.Find ]
        enabled: textEdit.activeFocus
        onActivated: findBar.open()
    }

    RowLayout {
        Layout.fillWidth: true
        Item { Layout.fillWidth: true }
        Rectangle {
            implicitHeight: 22
            implicitWidth: findRow.implicitWidth + 12
            radius: Theme.radiusSm
            color: findBar.visible ? Theme.accentSoft : (findHover.hovered ? Theme.surface2 : "transparent")
            RowLayout {
                id: findRow
                anchors.centerIn: parent
                spacing: 3
                MaterialIcon { name: "find_replace"; size: 12; color: findBar.visible ? Theme.accent : Theme.text2 }
                Text { text: qsTr("Find / Replace"); color: findBar.visible ? Theme.accent : Theme.text2; font.pixelSize: Theme.fontXs; font.weight: Font.DemiBold }
            }
            HoverHandler { id: findHover }
            TapHandler { onTapped: findBar.toggle() }
        }
    }
    FindReplaceBar {
        id: findBar
        editor: textEdit
    }
    NoteFormatToolbar {
        Layout.fillWidth: true
        editor: textEdit
        dialog: spellDialog
        spell: noteSpell.spell
    }
    Rectangle {
        Layout.fillWidth: true
        // The box has to clear the editor's own top/bottom padding as well as
        // the margins below, or the last line is clipped as the note grows.
        Layout.preferredHeight: Math.max(root.minimumHeight,
                                         textEdit.contentHeight + textEdit.topPadding
                                         + textEdit.bottomPadding + 23)
        radius: Theme.radiusSm
        color: Theme.surface
        border.color: textEdit.activeFocus ? Theme.accent : Theme.border
        TextArea {
            id: textEdit
            // Tab types a tab character, as it does in a word processor.
            // Declining the event hands it to the TextArea, which inserts the
            // tab through the document cursor, so it survives in rich text.
            // Ctrl+Tab and Shift+Tab still move focus out of the editor. On
            // macOS Qt reports the physical Control key as MetaModifier
            // (ControlModifier is Command, and Command+Tab is the app switcher).
            Keys.onTabPressed: (event) => {
                if (event.modifiers & (Qt.ControlModifier | Qt.MetaModifier))
                    nextItemInFocusChain().forceActiveFocus(Qt.TabFocusReason)
                else
                    event.accepted = false
            }
            Keys.onBacktabPressed: nextItemInFocusChain(false).forceActiveFocus(Qt.BacktabFocusReason)
            anchors.fill: parent
            anchors.margins: 9
            anchors.bottomMargin: 14
            color: Theme.text
            textFormat: TextEdit.RichText
            wrapMode: TextEdit.WordWrap
            selectByMouse: true
            persistentSelection: true
            background: null
            font.family: "Arial"
            // Match TextFormatter's Normal Text style exactly. Using
            // pixelSize here made the unformatted default (~9pt at
            // 96 DPI) visibly smaller than Normal Text's 12pt.
            font.pointSize: 12
            font.weight: Font.Normal
            placeholderText: root.placeholderText
            enabled: root.editingEnabled
            onTextChanged: {
                if (root._settingValue)
                    return
                root._lastEmittedValue = root.documentHtml()
                root.valueEdited(root._lastEmittedValue)
            }
            SpellCheckField {
                id: noteSpell
                dialog: spellDialog
                insertFields: root.insertFields
                onInsertFieldRequested: (path) => root.insertFieldRequested(path)
            }
            CaretFollow {}
        }
    }
    SpellCheckDialog { id: spellDialog }
}

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ProjectNotesDesktop

// Log Viewer — a separate, movable, non-modal top-level window (declared as a
// plain Window so it lives outside the main ApplicationWindow). One tab per
// *.log file in the app's logs folder; content streams in on background threads
// (LogViewerController / LogFileLoader) so even large logs load without blocking.
//
// Styled from Theme.qml so it matches the main window. The window is hidden (not
// destroyed) on close, so its tabs and loaders persist across reopenings.
Window {
    id: win

    title: qsTr("Log Viewer")
    width: 900
    height: 600
    minimumWidth: 420
    minimumHeight: 280
    color: Theme.bg

    // filePath -> { ta: TextArea, flick: Flickable } for content routing.
    property var _editors: ({})

    // The log file backing the currently selected tab (for Clear Log).
    readonly property string _currentFilePath:
        (tabsModel.count > 0 && tabBar.currentIndex >= 0
         && tabBar.currentIndex < tabsModel.count)
            ? tabsModel.get(tabBar.currentIndex).filePath : ""

    // Show + focus the window, starting the loaders on first open.
    function openViewer() {
        LogViewerController.start()
        win.show()
        win.raise()
        win.requestActivate()
    }

    // Keep the instance alive on close so tabs/loaders survive a reopen.
    onClosing: (close) => { close.accepted = false; win.hide() }

    function _registerEditor(filePath, ta, flick) {
        var m = win._editors
        m[filePath] = { ta: ta, flick: flick }
        win._editors = m
    }
    function _unregisterEditor(filePath) {
        var m = win._editors
        delete m[filePath]
        win._editors = m
    }

    // Content arrives from the background loader in small chunks (every ~30ms
    // while tailing, every ~100ms while backfilling — see LogFileLoader). QtQuick's
    // TextArea isn't viewport-virtualized like the Widgets QPlainTextEdit was, so
    // an insert() re-lays-out the whole growing block; calling it once per chunk
    // made big logs visibly stutter. Buffer chunks per file and flush them to the
    // TextArea on a coalesced timer instead, so the UI does far fewer, larger
    // inserts without changing the loader's own read pacing/threading.
    property var _pendingAppend: ({})
    property var _pendingPrepend: ({})

    Timer {
        id: flushTimer
        interval: 120
        repeat: true
        onTriggered: win._flushPendingLogContent()
    }

    function _queueAppend(filePath, content) {
        var m = win._pendingAppend
        m[filePath] = (m[filePath] || "") + content
        win._pendingAppend = m
        if (!flushTimer.running) flushTimer.start()
    }

    function _queuePrepend(filePath, content) {
        // Each new chunk is further back in the file than the last, so it must
        // land above what's already buffered — prepend within the buffer too.
        var m = win._pendingPrepend
        m[filePath] = content + (m[filePath] || "")
        win._pendingPrepend = m
        if (!flushTimer.running) flushTimer.start()
    }

    function _flushPendingLogContent() {
        var stillPending = false

        for (var fp in win._pendingAppend) {
            var content = win._pendingAppend[fp]
            if (content === "") continue
            var e = win._editors[fp]
            if (!e) { stillPending = true; continue }
            var fl = e.flick
            var atBottom = !fl || (fl.contentY >= fl.contentHeight - fl.height - 4)
            e.ta.insert(e.ta.length, content)
            win._pendingAppend[fp] = ""
            if (fl && atBottom)
                Qt.callLater((function (flick) {
                    return function () { flick.contentY = Math.max(0, flick.contentHeight - flick.height) }
                })(fl))
        }

        for (var fp2 in win._pendingPrepend) {
            var content2 = win._pendingPrepend[fp2]
            if (content2 === "") continue
            var e2 = win._editors[fp2]
            if (!e2) { stillPending = true; continue }
            var fl2 = e2.flick
            var before = fl2 ? fl2.contentHeight : 0
            e2.ta.insert(0, content2)
            win._pendingPrepend[fp2] = ""
            // Keep the same lines in view: shift down by the height just added.
            if (fl2)
                Qt.callLater((function (flick, beforeHeight) {
                    return function () {
                        var delta = flick.contentHeight - beforeHeight
                        flick.contentY = Math.min(flick.contentY + delta,
                                                   Math.max(0, flick.contentHeight - flick.height))
                    }
                })(fl2, before))
        }

        if (!stillPending)
            flushTimer.stop()
    }

    ListModel { id: tabsModel }   // { filePath, fileName }

    Connections {
        target: LogViewerController

        function onLogFileOpened(filePath, fileName) {
            for (var i = 0; i < tabsModel.count; ++i)
                if (tabsModel.get(i).filePath === filePath)
                    return
            tabsModel.append({ filePath: filePath, fileName: fileName })
        }

        function onContentAppended(filePath, content) {
            win._queueAppend(filePath, content)
        }

        function onContentPrepended(filePath, content) {
            win._queuePrepend(filePath, content)
        }

        function onLogFileClosed(filePath) {
            for (var i = 0; i < tabsModel.count; ++i)
                if (tabsModel.get(i).filePath === filePath) {
                    tabsModel.remove(i)
                    break
                }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Tab strip ───────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 40
            color: Theme.surface
            visible: tabsModel.count > 0

            Rectangle {   // bottom divider
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: Theme.border
            }

            TabBar {
                id: tabBar
                anchors.fill: parent
                anchors.leftMargin: 8
                spacing: 4
                background: null

                Repeater {
                    model: tabsModel
                    delegate: TabButton {
                        id: tabBtn
                        required property var model
                        required property int index
                        implicitHeight: 40
                        implicitWidth: Math.max(90, tabLabel.implicitWidth + 28)
                        padding: 0

                        contentItem: Text {
                            id: tabLabel
                            text: tabBtn.model.fileName
                            color: tabBtn.checked ? Theme.text : Theme.text2
                            font.pixelSize: 12
                            font.weight: tabBtn.checked ? Font.DemiBold : Font.Normal
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: "transparent"
                            Rectangle {   // active underline
                                anchors.bottom: parent.bottom
                                width: parent.width; height: 2
                                color: tabBtn.checked ? Theme.accent : "transparent"
                            }
                        }
                    }
                }
            }
        }

        // ── Tab content ─────────────────────────────────────────────────────────
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            Repeater {
                model: tabsModel
                delegate: Item {
                    required property var model

                    Flickable {
                        id: flick
                        anchors.fill: parent
                        anchors.margins: 1
                        clip: true
                        contentWidth: logText.paintedWidth
                        contentHeight: logText.paintedHeight
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                        ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                        TextArea {
                            id: logText
                            width: Math.max(implicitWidth, flick.width)
                            readOnly: true
                            wrapMode: TextArea.NoWrap
                            textFormat: TextArea.PlainText
                            selectByMouse: true
                            persistentSelection: true
                            font.family: "monospace"
                            font.pixelSize: 12
                            color: Theme.text
                            background: Rectangle { color: Theme.bg }
                            Component.onCompleted:
                                win._registerEditor(model.filePath, logText, flick)
                            Component.onDestruction:
                                win._unregisterEditor(model.filePath)
                        }
                    }
                }
            }
        }

        // Empty state.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: tabsModel.count === 0
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                MaterialIcon {
                    name: "description"; size: 40; color: Theme.text3
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: qsTr("No log files yet")
                    color: Theme.text2; font.pixelSize: 14; font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: LogViewerController.logFolder()
                    color: Theme.text3; font.pixelSize: 11
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // ── Footer ──────────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 52
            color: Theme.surface

            Rectangle {   // top divider
                anchors.top: parent.top
                width: parent.width; height: 1
                color: Theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10

                Item { Layout.fillWidth: true }

                // Clear Log — deletes the current tab's log file.
                Rectangle {
                    implicitHeight: 32
                    implicitWidth: clearRow.implicitWidth + 24
                    radius: Theme.radiusSm
                    enabled: win._currentFilePath !== ""
                    opacity: enabled ? 1.0 : 0.4
                    color: clearHover.hovered ? Theme.redSoft : Theme.surface
                    border.color: clearHover.hovered ? Theme.red : Theme.border
                    RowLayout {
                        id: clearRow
                        anchors.centerIn: parent
                        spacing: 6
                        MaterialIcon { name: "delete"; size: 16; color: Theme.red }
                        Text { text: qsTr("Clear Log"); color: Theme.red; font.pixelSize: 12 }
                    }
                    HoverHandler { id: clearHover; enabled: parent.enabled }
                    TapHandler {
                        enabled: parent.enabled
                        onTapped: LogViewerController.clearLog(win._currentFilePath)
                    }
                }

                // Close — hides the window (loaders keep running).
                Rectangle {
                    implicitHeight: 32
                    implicitWidth: closeText.implicitWidth + 28
                    radius: Theme.radiusSm
                    color: closeHover.hovered ? Theme.accentStrong : Theme.accent
                    Text {
                        id: closeText
                        anchors.centerIn: parent
                        text: qsTr("Close")
                        color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold
                    }
                    HoverHandler { id: closeHover }
                    TapHandler { onTapped: win.hide() }
                }
            }
        }
    }
}

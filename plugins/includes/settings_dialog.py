# Copyright (C) 2026 Paul McKinney
"""Common geometry and dismissal handling for native plugin dialogs."""
from PyQt6.QtWidgets import QApplication, QDialog
from includes.common import ProjectNotesCommon


class PluginSettingsDialog(QDialog):
    def restore_window_state(self):
        settings = ProjectNotesCommon()
        try:
            w = int(settings.get_plugin_setting("W", self.settings_pluginname))
            h = int(settings.get_plugin_setting("H", self.settings_pluginname))
            if w > 0 and h > 0:
                self.resize(w, h)
        except (TypeError, ValueError):
            pass
        self.center_on_main_window()

    def center_on_main_window(self):
        main_window = QApplication.activeWindow()
        if main_window:
            geometry = main_window.geometry()
            x = geometry.x() + (geometry.width() - self.width()) // 2
            y = geometry.y() + (geometry.height() - self.height()) // 2
            self.move(max(0, x), max(0, y))

    def save_window_state(self):
        settings = ProjectNotesCommon()
        settings.set_plugin_setting("W", self.settings_pluginname, str(self.width()))
        settings.set_plugin_setting("H", self.settings_pluginname, str(self.height()))

    def reject_changes(self):
        self.reject()

    def reject(self):
        self.save_window_state()
        super().reject()


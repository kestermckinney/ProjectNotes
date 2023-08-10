// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PLUGINSETTINGSDIALOG_H
#define PLUGINSETTINGSDIALOG_H

#include "pnpluginmanager.h"

#include <QDialog>
#include <QListWidgetItem>
#include <QTableWidgetItem>

namespace Ui {
class PluginSettingsDialog;
}

class PluginSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginSettingsDialog(QWidget *parent = nullptr);
    ~PluginSettingsDialog();

    void editPluginSettings(PNPluginManager* t_pluginmanager);
    void selectPlugin(int t_index);

private slots:
    void on_PluginsListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_EnabledCheckBox_stateChanged(int arg1);

    void on_SettingsTableWidget_cellChanged(int row, int column);


private:
    Ui::PluginSettingsDialog *ui;

    bool m_loading = false; // loading values don't respond to events
    PNPlugin* m_current_selection = nullptr;
    PNPluginManager* m_plugin_manager = nullptr;
};

#endif // PLUGINSETTINGSDIALOG_H

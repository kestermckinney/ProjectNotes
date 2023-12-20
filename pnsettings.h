// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNSETTINGS_H
#define PNSETTINGS_H

#include <QSettings>
#include <QMainWindow>
#include <QTableView>

#include "pnspellchecker.h"

class PNSettings
{
public:
    PNSettings();
    ~PNSettings();

    QVariant getPluginSetting(const QString& t_plugin_name, const QString& t_parameter_name);
    void setPluginSetting(const QString& t_plugin_name, const QString& t_parameter_name, const QString& t_parameter_value);
    bool getPluginEnabled(const QString& t_plugin_name);
    void setPluginEnabled(const QString& t_plugin_name, bool t_enabled);

    QVariant getLastDatabase();
    void setLastDatabase(const QString& t_last_database);

    QVariant getWindowStateData(const QString& t_state_data_name);
    void setWindowStateData(const QString& t_state_data_name, const QVariant& t_data);

    void setWindowState(const QString& t_window_name, const QWidget& t_window);
    bool getWindowState(const QString& t_window_name, QWidget& t_window);

    QString getDefaultDictionary();
    void setDefaultDictionary(const QString& t_dictionary);

    QString getPersonalDictionary();
    void setPersonalDictionary(const QString& t_dictionary);

    void setTableViewState(const QString& t_view_name, const QTableView& t_view);
    bool getTableViewState(const QString& t_view_name, QTableView& t_view);

    void setTableSortColumn(const QString& t_view_name, const int t_column, const QString t_direction);
    bool getTableSortColumn(const QString& t_view_name, int& t_column, QString& t_direction);
    int getStoredInt(const QString& t_value_name);
    void setStoredInt(const QString& t_value_name, int t_int_value);

    PNSpellChecker* spellchecker();

private:
    int getWindowX(const QString& t_window_name);
    int getWindowY(const QString& t_window_name);
    void setWindowX(const QString& t_window_name, int t_x);
    void setWindowY(const QString& t_window_name, int t_y);

    int getWindowWidth(const QString& t_window_name);
    int getWindowHeight(const QString& t_window_name);
    bool getWindowMaximized(const QString& t_window_name);

    void setWindowWidth(const QString& t_window_name, int t_idth);
    void setWindowHeight(const QString& t_window_name, int t_height);
    void setWindowMaximized(const QString& t_window_name, bool t_maximized);

    void setWindowStatusBar(const QString& t_window_name, bool t_status_bar);
    bool getWindowStatusBar(const QString& t_window_name);

private:
    QSettings* m_app_config = nullptr;
    QSettings* m_plugin_config = nullptr;

    PNSpellChecker* m_spellchecker = nullptr;
};

static PNSettings global_Settings;

#endif // PNSETTINGS_H

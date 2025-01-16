// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pluginsettingsdialog.h"
#include "ui_pluginsettingsdialog.h"
#include "pnpluginmanager.h"
#include "pnplugin.h"
#include "pnsettings.h"

#include <QList>
#include <QTableWidgetItem>
//#include <QDebug>

PluginSettingsDialog::PluginSettingsDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::PluginSettingsDialog)
{
    ui->setupUi(this);

    ui->SettingsTableWidget->verticalHeader()->setDefaultSectionSize(15);

    //TODO: allow for passwords to stay hidden and ecrypted
}

void PluginSettingsDialog::editPluginSettings(PNPluginManager* t_pluginmanager)
{
    m_plugin_manager = t_pluginmanager;

    // TODO: MULTI-THREAD Can't call this in the middle of loading
    // if a reload occurs this list of pointers will all be off

    // TODO: MULTI-THREAD Maybe we can lock the plugin manager from reloading when this method is called
    // the plugin settings dialog still needs to be part of the main GUI thread, so it doesn't pause the python plugin manager

    QList<PNPlugin*> plist = m_plugin_manager->getPlugins();

    ui->PluginsListWidget->clear();

    for (PNPlugin* p : plist)
    {
        QListWidgetItem* qli = new QListWidgetItem;
        qli->setText(p->getPNPluginName());
        qli->setSizeHint(QSize(ui->PluginsListWidget->width(), 15));
        ui->PluginsListWidget->addItem(qli);
    }

    if (ui->PluginsListWidget->currentRow() > -1)
        selectPlugin(ui->PluginsListWidget->currentRow());

    setModal(true);
    exec();
}

void PluginSettingsDialog::selectPlugin(int t_index)
{
    m_loading = true;

    //TODO: MULTI-THREAD Wait on pluginmanager loading this call
    // if it has reloaded this function will fail becuase list of pointers will have changed
    QList<PNPlugin*> plist = m_plugin_manager->getPlugins();
    m_current_selection = plist[t_index];

    // description
    ui->PluginDescriptionPlainTextEdit->setPlainText(m_current_selection->getPNPluginDescription());

    // is enabled 
    m_current_selection->setEnabled(m_current_selection->isEnabled());
    ui->EnabledCheckBox->setCheckState( m_current_selection->isEnabled() ? Qt::Checked : Qt::Unchecked);

    // list of events
    ui->EventsListWidget->clear();
    ui->EventsListWidget->addItems(m_current_selection->getEventNames());

    // list of properties
    ui->SettingsTableWidget->clear();

    ui->SettingsTableWidget->setColumnCount(2);
    QTableWidgetItem* propitem = new QTableWidgetItem("Property");
    propitem->setBackground(Qt::gray);
    QTableWidgetItem* propval = new QTableWidgetItem("Setting");
    ui->SettingsTableWidget->setHorizontalHeaderItem(0, propitem);
    ui->SettingsTableWidget->setHorizontalHeaderItem(1, propval);
    ui->SettingsTableWidget->verticalHeader()->hide();

    // set the location
    ui->LocationLineEdit->setText(m_current_selection->getPluginLocation());

    QStringList qs = m_current_selection->getParameterNames();
    ui->SettingsTableWidget->setRowCount(qs.length());

    for (int i = 0; i < qs.length(); i++)
    {
        QTableWidgetItem* item = new QTableWidgetItem(qs[i]);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setBackground(Qt::gray);

        ui->SettingsTableWidget->setItem(i, 0, item);
        ui->SettingsTableWidget->horizontalHeader()->setStretchLastSection(true);

        QVariant val = global_Settings.getPluginSetting(m_current_selection->getPNPluginName(), qs[i]);
        QTableWidgetItem* setting = new QTableWidgetItem(val.toString());
        ui->SettingsTableWidget->setItem(i, 1, setting);
    }

    ui->SettingsTableWidget->horizontalHeader()->setStretchLastSection(true);

    m_loading = false;
}

PluginSettingsDialog::~PluginSettingsDialog()
{
    delete ui;
}

void PluginSettingsDialog::on_PluginsListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);

    if (m_loading) return; // loading values ignore events
    if (ui->PluginsListWidget->currentRow() > -1)
        selectPlugin(ui->PluginsListWidget->currentRow());
}


void PluginSettingsDialog::on_EnabledCheckBox_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    if (m_loading) return; // loading values ignore events

    m_current_selection->setEnabled((ui->EnabledCheckBox->checkState() == Qt::Checked));
}


void PluginSettingsDialog::on_SettingsTableWidget_cellChanged(int row, int column)
{
    if (m_loading) return; // loading values ignore events
    // TODO: MULTI-THREAD if the list of plugins get reloaded this function would fail below

    if (m_current_selection)
        global_Settings.setPluginSetting(m_current_selection->getPNPluginName(), ui->SettingsTableWidget->item(row, 0)->text(), ui->SettingsTableWidget->item(row, column)->text());
}

#include "pluginsettingsdialog.h"
#include "ui_pluginsettingsdialog.h"
#include "pnpluginmanager.h"
#include "pnplugin.h"

#include <QList>
#include <QTableWidgetItem>
#include <QDebug>

PluginSettingsDialog::PluginSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginSettingsDialog)
{
    ui->setupUi(this);

    ui->SettingsTableWidget->setColumnCount(2);
    QTableWidgetItem* propitem = new QTableWidgetItem("Property");
    propitem->setBackground(Qt::gray);
    QTableWidgetItem* propval = new QTableWidgetItem("Setting");
    ui->SettingsTableWidget->setHorizontalHeaderItem(0, propitem);
    ui->SettingsTableWidget->setHorizontalHeaderItem(1, propval);
    ui->SettingsTableWidget->verticalHeader()->hide();

//    QTableWidgetItem* item1 = new QTableWidgetItem("Item 1");
//    item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
//    item1->setBackground(Qt::gray);
//    ui->SettingsTableWidget->setRowCount(2);
//    ui->SettingsTableWidget->setItem(0, 0, item1);

    //TODO: allow for passwords to stay hidden and ecrypted
}

void PluginSettingsDialog::editPluginSettings(PNPluginManager* t_pluginmanager)
{
    m_plugin_manager = t_pluginmanager;

    QList<PNPlugin*> plist = m_plugin_manager->getPlugins();

    ui->PluginsListWidget->clear();

    for (PNPlugin* p : plist)
    {
        ui->PluginsListWidget->addItem(p->getPNPluginName());
    }

    if (ui->PluginsListWidget->currentRow() > -1)
        selectPlugin(ui->PluginsListWidget->currentRow());

    setModal(true);
    exec();
}

void PluginSettingsDialog::selectPlugin(int t_index)
{
    m_loading = true;

    QList<PNPlugin*> plist = m_plugin_manager->getPlugins();
    PNPlugin* p = plist[t_index];

    // description
    ui->PluginDescriptionLineEdit->setText(p->getPNPluginDescription());

    // is enabled
    ui->EnabledCheckBox->setCheckState( p->isEnabled() ? Qt::Checked : Qt::Unchecked);

    // list of events
    ui->EventsListWidget->clear();
    ui->EventsListWidget->addItems(p->getEventNames());

    // list of properties
    ui->SettingsTableWidget->clear();

    // set the location
    ui->LocationLineEdit->setText(p->getPluginLocation());

    QStringList qs = p->getParameterNames();
    ui->SettingsTableWidget->setRowCount(qs.length());

    for (int i = 0; i < qs.length(); i++)
    {
        QTableWidgetItem* item = new QTableWidgetItem(qs[i]);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setBackground(Qt::gray);
        ui->SettingsTableWidget->setItem(i, 0, item);
        ui->SettingsTableWidget->horizontalHeader()->setStretchLastSection(true);

        //TODO: get theplugin parameter value
        QTableWidgetItem* setting = new QTableWidgetItem("THE VALUE");
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
    if (m_loading) return; // loading values ignore events
    if (ui->PluginsListWidget->currentRow() > -1)
        selectPlugin(ui->PluginsListWidget->currentRow());
}


void PluginSettingsDialog::on_EnabledCheckBox_stateChanged(int arg1)
{
    if (m_loading) return; // loading values ignore events
    QString val = QString("%1").arg(ui->EnabledCheckBox->checkState());
    qDebug() << val;
    // TODO: Store state value
}


void PluginSettingsDialog::on_SettingsTableWidget_cellChanged(int row, int column)
{
    if (m_loading) return; // loading values ignore events
    qDebug() << ui->SettingsTableWidget->item(row, column)->text();

    // TODO: Store state value
}




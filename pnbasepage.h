// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNBASEPAGE_H
#define PNBASEPAGE_H

#include <QWidget>
#include "pntableview.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class PNBasePage : public QWidget
{
    Q_OBJECT
public:
    explicit PNBasePage(QWidget *parent = nullptr);

    virtual void setupModels( Ui::MainWindow *t_ui ) { Q_UNUSED(t_ui)}
    virtual void setPageModel( PNSqlQueryModel* t_page_model ) { m_page_model = t_page_model; }
    virtual void newRecord();
    virtual void copyItem();
    virtual void openItem();
    virtual void deleteItem();
    virtual void setPageTitle();
    QString getHistoryText() { return m_history_text; }
    void setHistoryText(QString t_history_text) { m_history_text = t_history_text; }
    virtual void saveState();
    virtual void loadState();
    virtual void openRecord(QVariant& t_record_id);
    virtual void submitRecord() { };

    PNSortFilterProxyModel*  getCurrentModel() { return m_current_model; }
    PNTableView* getCurrentView() { return m_current_view; }
    void setCurrentModel( PNSortFilterProxyModel* t_current_model ) { m_current_model = t_current_model; }
    void setCurrentView( PNTableView* t_current_view ) { m_current_view = t_current_view; }
    virtual void setButtonAndMenuStates();
    const QString getTableName() { return m_table_name; }
    void setTableName(const QString& t_table_name) { m_table_name = t_table_name; }
    void buildPluginMenu(PluginManager* t_pm, QMenu* t_menu);
    void setRecordId(QVariant t_record_id) { m_record_id = t_record_id; }
    QVariant getRecordId() { return m_record_id; }

public slots:
    void slotPluginMenu(Plugin* t_plugin, const QString& t_functionname);

private:
    PNSortFilterProxyModel* m_current_model = nullptr;
    PNSqlQueryModel* m_page_model = nullptr;
    PNTableView* m_current_view = nullptr;

    QString m_table_name;

    QVariant m_record_id;
    QString m_history_text;
};

#endif // PNBASEPAGE_H

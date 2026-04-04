// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QWidget>
#include "tableview.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class BasePage : public QWidget
{
    Q_OBJECT
public:
    explicit BasePage(QWidget *parent = nullptr);

    virtual void setupModels( Ui::MainWindow *ui ) { Q_UNUSED(ui)}
    virtual void setPageModel( SqlQueryModel* pageModel ) { m_pageModel = pageModel; }
    virtual void newRecord();
    virtual void copyItem();
    virtual void openItem();
    virtual void deleteItem();
    virtual void setPageTitle();
    QString getHistoryText() { return m_historyText; }
    void setHistoryText(QString historyText) { m_historyText = historyText; }
    virtual void saveState();
    virtual void loadState();
    virtual void openRecord(QVariant& recordId);
    virtual void submitRecord() { };

    SortFilterProxyModel*  getCurrentModel() { return m_currentModel; }
    TableView* getCurrentView() { return m_currentView; }
    void setCurrentModel( SortFilterProxyModel* currentModel ) { m_currentModel = currentModel; }
    void setCurrentView( TableView* currentView ) { m_currentView = currentView; }
    virtual void setButtonAndMenuStates();
    const QString getTableName() { return m_tableName; }
    void setTableName(const QString& tableName) { m_tableName = tableName; }
    virtual void buildPluginMenu(PluginManager* pm, QMenu* menu);
    void setRecordId(QVariant recordId) { m_recordId = recordId; }
    QVariant getRecordId() { return m_recordId; }
    QVariantList getSelectedRecordIds();

public slots:
    void slotPluginMenu(Plugin* plugin, const QString& functionname, const QString& tablefilter, const QString& parameter);

private:
    SortFilterProxyModel* m_currentModel = nullptr;
    SqlQueryModel* m_pageModel = nullptr;
    TableView* m_currentView = nullptr;

    QString m_tableName;

    QVariant m_recordId;
    QString m_historyText;
};

#endif // BASEPAGE_H

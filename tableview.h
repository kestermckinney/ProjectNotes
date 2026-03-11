// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include "filterdatadialog.h"
#include "plugin.h"
#include "pluginmanager.h"

#include <QTableView>
#include <QObject>
#include <QItemDelegate>

class NotEditableDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit NotEditableDelegate(QObject *parent = 0)
        : QItemDelegate(parent)
    {}

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
    { Q_UNUSED(model); Q_UNUSED(option); Q_UNUSED(index); Q_UNUSED(event); return false; }
    QWidget* createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const
    { return Q_NULLPTR; }

};

class TableView : public QTableView
{
    Q_OBJECT

public:
    TableView(QWidget* parent = nullptr);
    ~TableView();
    void setModel(QAbstractItemModel *model) override;
    void filterDialog() { this->slotFilterRecords(); }
    bool eventFilter(QObject *watched, QEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *e) override;

    bool getHasOpen() { return m_hasOpen; }
    void setHasOpen(bool hasOpen) { m_hasOpen = hasOpen; }
    void setKeyToOpenField(int keyToOpenField) { m_keyToOpenField = keyToOpenField; }

signals:
    void signalOpenRecordWindow(QVariant record_id);

public slots:
    virtual void dataRowSelected(const QModelIndex &index);
    virtual void dataRowActivated(const QModelIndex &index);
    virtual void slotNewRecord();
    void slotDeleteRecord();
    void slotCopyRecord();
    void slotOpenRecord();
    void slotExportRecord();
    void slotFilterRecords();
    void slotRefreshRecords();
    void slotResetColumns();
    void slotPluginMenu(Plugin* plugin, const QString& functionname, const QString& exportname, const QString& tablefilter, const QString& parameter);

private:
    void sortMenu(QMenu* menu);

    QPoint m_pressPos;
    bool m_isMoving = false;

    QAction *m_newRecord;
    QAction *m_deleteRecord;
    QAction *m_openRecord;
    QAction *m_exportRecord;
    QAction *m_filterRecords;
    QAction *m_refreshRecords;
    QAction *m_resetColumns;
    QAction *m_copyRecord;

    bool m_hasOpen = false;

    FilterDataDialog *m_filterdialog = nullptr;
    int m_keyToOpenField = 0;
    QItemSelection m_savedSelection;
};

#endif // TABLEVIEW_H

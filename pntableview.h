// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNTABLEVIEW_H
#define PNTABLEVIEW_H

#include "filterdatadialog.h"
#include "pnplugin.h"
#include "pnpluginmanager.h"

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

class PNTableView : public QTableView
{
    Q_OBJECT

public:
    PNTableView(QWidget* t_parent = nullptr);
    ~PNTableView();
    void setModel(QAbstractItemModel *t_model) override;
    void filterDialog() { this->slotFilterRecords(); }
    bool eventFilter(QObject *t_watched, QEvent *t_event) override;

    void contextMenuEvent(QContextMenuEvent *t_e) override;

    bool getHasOpen() { return m_has_open; }
    void setHasOpen(bool t_has_open) { m_has_open = t_has_open; }
    void setKeyToOpenField(int t_key_to_open_field) { m_key_to_open_field = t_key_to_open_field; }

signals:
    void signalOpenRecordWindow(QVariant record_id);

public slots:
    virtual void dataRowSelected(const QModelIndex &t_index);
    virtual void dataRowActivated(const QModelIndex &t_index);
    void slotNewRecord();
    void slotDeleteRecord();
    void slotCopyRecord();
    void slotOpenRecord();
    void slotExportRecord();
    void slotFilterRecords();
    void slotRefreshRecords();
    void slotResetColumns();  
    void slotPluginMenu(PNPlugin* t_plugin);

private:
    QPoint m_pressPos;
    bool m_isMoving = false;

    QAction *newRecord;
    QAction *deleteRecord;
    QAction *openRecord;
    QAction *exportRecord;
    QAction *filterRecords;
    QAction *refreshRecords;
    QAction *resetColumns;
    QAction *copyRecord;

    bool m_has_open = false;

    FilterDataDialog *m_filterdialog = nullptr;
    int m_key_to_open_field = 0;
};

#endif // PNTABLEVIEW_H

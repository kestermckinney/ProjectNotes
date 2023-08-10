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
    { return false; }
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

signals:
    void signalOpenRecordWindow();

public slots:
    virtual void dataRowSelected(const QModelIndex &t_index);
    virtual void dataRowActivated(const QModelIndex &t_index);
    void slotNewRecord();
    void slotDeleteRecord();
    void slotCopyRecord();
    void slotOpenRecord();
    void slotExportRecord();
    void slotFilterRecords();
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
    QAction *resetColumns;
    QAction *copyRecord;

    bool m_has_open = false;

    FilterDataDialog *m_filterdialog = nullptr;
};

#endif // PNTABLEVIEW_H

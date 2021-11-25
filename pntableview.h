#ifndef PNTABLEVIEW_H
#define PNTABLEVIEW_H

#include "pndatabaseobjects.h"
#include "pnsettings.h"

#include <QTableView>
#include <QObject>

class PNTableView : public QTableView
{
public:
    PNTableView(QWidget* parent = nullptr);
    ~PNTableView();
    void setModel(QAbstractItemModel *model) override;
    // BUGGY TODO REMOVE bool eventFilter(QObject *watched, QEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *e) override;

public slots:
    virtual void dataRowSelected(const QModelIndex &index);
    virtual void dataRowActivated(const QModelIndex &index);
    void slotNewRecord();
    void slotDeleteRecord();
    void slotOpenRecord();
    void slotExportRecord();
    void slotFilterRecords();
    void slotResetColumns();

private:
    QPoint m_pressPos;
    bool m_isMoving = false;

    QAction *newRecord;
    QAction *deleteRecord;
    QAction *openRecord;
    QAction *exportRecord;
    QAction *filterRecords;
    QAction *resetColumns;
};

#endif // PNTABLEVIEW_H

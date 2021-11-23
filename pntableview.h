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
    bool eventFilter(QObject *watched, QEvent *event) override;

    // override slots STOPPED HERE
    //void selectRow(int row); --- slot
    //void doubleClicked(const QModelIndex &index);
    //void QAbstractItemView::clicked(const QModelIndex &index)

    void contextMenuEvent(QContextMenuEvent *e) override;

    //signals doubleClicked and clicked and selectRow

public slots:
    virtual void dataRowSelected(const QModelIndex &index);
    virtual void dataRowActivated(const QModelIndex &index);
    void slotNewRecord();
    void slotDeleteRecord();
    void slotOpenRecord();
    void slotExportRecord();
    void slotFilterRecords();

private:
    QPoint m_pressPos;
    bool m_isMoving = false;

    QAction *newRecord;
    QAction *deleteRecord;
    QAction *openRecord;
    QAction *exportRecord;
    QAction *filterRecords;
};

#endif // PNTABLEVIEW_H

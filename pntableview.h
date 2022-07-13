#ifndef PNTABLEVIEW_H
#define PNTABLEVIEW_H

#include "filterdatadialog.h"
#include "pndatabaseobjects.h"
#include "pnsettings.h"

#include <QTableView>
#include <QObject>

class PNTableView : public QTableView
{
    Q_OBJECT

public:
    PNTableView(QWidget* parent = nullptr);
    ~PNTableView();
    void setModel(QAbstractItemModel *model) override;
    void filterDialog() { this->slotFilterRecords(); }
    // BUGGY TODO REMOVE bool eventFilter(QObject *watched, QEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *e) override;

signals:
    void signalOpenRecordWindow();

public slots:
    virtual void dataRowSelected(const QModelIndex &index);
    virtual void dataRowActivated(const QModelIndex &index);
    void slotNewRecord();
    void slotDeleteRecord();
    void slotCopyRecord();
    void slotOpenRecord();
    void slotExportRecord();
    void slotFilterRecords();
    void slotResetColumns();

    bool getHasOpen() { return m_has_open; }
    void setHasOpen(bool t_has_open) { m_has_open = t_has_open; }

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
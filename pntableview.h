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

private:
    QPoint m_pressPos;
    bool m_isMoving = false;
};

#endif // PNTABLEVIEW_H

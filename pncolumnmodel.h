#ifndef PNCOLUMNMODEL_H
#define PNCOLUMNMODEL_H

#include "pnsqlquerymodel.h"
#include <QObject>

class PNColumnModel : public PNSqlQueryModel
{
public:
    PNColumnModel(QObject *parent);
    void setColumnModel(PNSqlQueryModel* columnmodel);
    PNSqlQueryModel* columnmodel() { return m_ColumnModel; };

private:
    PNSqlQueryModel* m_ColumnModel;
};

#endif // PNCOLUMNMODEL_H

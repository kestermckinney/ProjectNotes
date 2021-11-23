#ifndef VALUESELECTMODEL_H
#define VALUESELECTMODEL_H

#include "pnsqlquerymodel.h"
#include <QObject>

class ValueSelectModel : public PNSqlQueryModel
{
public:
    ValueSelectModel(QObject *parent);
    void setValuesColumn(QString Column);

    void setFilteringModel(PNSqlQueryModel* model) { m_FilteringModel = model; };

private:
    PNSqlQueryModel* m_FilteringModel;
};

#endif // VALUESELECTMODEL_H

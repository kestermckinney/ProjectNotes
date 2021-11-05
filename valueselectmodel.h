#ifndef VALUESELECTMODEL_H
#define VALUESELECTMODEL_H

#include "pnsqlquerymodel.h"
#include <QObject>

class ValueSelectModel : public PNSqlQueryModel
{
public:
    ValueSelectModel(QObject *parent);
    void setValuesModel(PNSqlQueryModel* model, QString Column);
};

#endif // VALUESELECTMODEL_H

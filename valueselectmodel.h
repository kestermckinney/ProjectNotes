#ifndef VALUESELECTMODEL_H
#define VALUESELECTMODEL_H

#include "pnsqlquerymodel.h"
#include <QObject>

class ValueSelectModel : public PNSqlQueryModel
{
public:
    ValueSelectModel(QObject *t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ValueSelectModel(this)); };
    void setValuesColumn(QString t_column);

    void setFilteringModel(PNSqlQueryModel* t_model) { m_filtering_model = t_model; };

private:
    PNSqlQueryModel* m_filtering_model;
};

#endif // VALUESELECTMODEL_H

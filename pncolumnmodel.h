#ifndef PNCOLUMNMODEL_H
#define PNCOLUMNMODEL_H

#include "FilterSaveStructure.h"

#include "pnsqlquerymodel.h"
#include <QObject>

class PNColumnModel : public PNSqlQueryModel
{
public:
    PNColumnModel(QObject *parent);
    void setColumnModel(PNSqlQueryModel* columnmodel);
    void setSavedFilters(QHash<QString, FilterSaveStructure>* filters) { savedFilters = filters; };
    void setFilteringModel(PNSqlQueryModel* model) { m_FilteringModel = model; };
    PNSqlQueryModel* columnmodel() { return m_ColumnModel; };
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    PNSqlQueryModel* m_ColumnModel;
    QHash<QString, FilterSaveStructure>* savedFilters;
    PNSqlQueryModel* m_FilteringModel;
};

#endif // PNCOLUMNMODEL_H

#ifndef PNCOLUMNMODEL_H
#define PNCOLUMNMODEL_H

#include "FilterSaveStructure.h"

#include "pnsqlquerymodel.h"
#include <QObject>

class PNColumnModel : public PNSqlQueryModel
{
public:
    PNColumnModel(QObject *t_parent);
    void setColumnModel(PNSqlQueryModel* t_columnmodel);
    void setSavedFilters(QHash<QString, FilterSaveStructure>* t_filters) { m_saved_filters = t_filters; };
    void setFilteringModel(PNSqlQueryModel* t_model) { m_filtering_model = t_model; };
    PNSqlQueryModel* columnmodel() { return m_column_model; };
    QVariant data(const QModelIndex &t_index, int t_role = Qt::DisplayRole) const override;

private:
    PNSqlQueryModel* m_column_model;
    QHash<QString, FilterSaveStructure>* m_saved_filters;
    PNSqlQueryModel* m_filtering_model;
};

#endif // PNCOLUMNMODEL_H

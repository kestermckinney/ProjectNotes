#ifndef COLUMNVIEW_H
#define COLUMNVIEW_H

#include "filterdatadialog.h"
#include "pntableview.h"
#include "valueselectmodel.h"
#include <QObject>

class ColumnView : public PNTableView
{
public:
    ColumnView(QWidget* parent = nullptr);
    void setColumnValuesModel( ValueSelectModel* model ) { valuesModel = model; };
    void setFilteredModel( PNSqlQueryModel* model ) { filteredModel = model; };
    void setValuesView( PNTableView* view, QHash<QString, FilterSaveStructure>* savedfilters ) { valuesView = view; savedFilters = savedfilters; };
    void setUI( FilterDataDialog* parent_ui ) { ParentUI = parent_ui; };

private:
    ValueSelectModel* valuesModel;
    PNSqlQueryModel* filteredModel;
    FilterDataDialog *ParentUI;

    PNTableView* valuesView;
    QHash<QString, FilterSaveStructure>* savedFilters;

public slots:
    void dataRowSelected(const QModelIndex &index) override;
};

#endif // COLUMNVIEW_H

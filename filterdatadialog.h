#ifndef FILTERDATADIALOG_H
#define FILTERDATADIALOG_H

#include "pnsqlquerymodel.h"
#include "pncolumnmodel.h"
#include "valueselectmodel.h"
#include "pnsortfilterproxymodel.h"

#include <QDialog>

namespace Ui {
class FilterDataDialog;
}

class FilterSaveStructure
{
public:
    QStringList m_ColumnValues;
    QString m_SearchString;
    QString m_SearchBeginValue;
    QString m_SearchEndValue;
};

class FilterDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDataDialog(QWidget *parent = nullptr);
    ~FilterDataDialog();

    void setFilterModel(PNSqlQueryModel* model);

private:
    Ui::FilterDataDialog *ui;

    PNColumnModel* columnModel = nullptr;
    PNSortFilterProxyModel* columnProxyModel = nullptr;
    ValueSelectModel* valuesModel = nullptr;
    PNSortFilterProxyModel* valuesProxyModel = nullptr;

    QString m_FilterName;

    bool m_TextSearch;
    int m_SelectedColumn = -1;  // nothing selected until construction

    QHash<QString, FilterSaveStructure> m_SavedFilters;
};

#endif // FILTERDATADIALOG_H

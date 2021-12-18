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
    QVariantList ColumnValues;
    QVariant SearchString;
    QVariant SearchBeginValue;
    QVariant SearchEndValue;
};

class FilterDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDataDialog(QWidget *parent = nullptr);
    ~FilterDataDialog();

    void setFilterModel(PNSqlQueryModel* model);
    void setEndValue(QVariant& text);
    void setBeginValue(QVariant& text);
    void setSearchText(QVariant& text);
    QVariant getEndValue();
    QVariant getStartValue();
    QVariant getSearchText();

private slots:
    void on_lineEditSearchText_textEdited(const QString &arg1);

    void on_lineEditStartValue_textEdited(const QString &arg1);

    void on_lineEditEndValue_textEdited(const QString &arg1);

    void on_pushButtonApply_clicked();

    void on_pushButtonCancel_clicked();

    void on_pushButtonAll_clicked();

    void on_pushButtonReset_clicked();

private:
    void setupFilters();
    Ui::FilterDataDialog *ui;

    PNColumnModel* columnModel = nullptr;
    PNSortFilterProxyModel* columnProxyModel = nullptr;
    ValueSelectModel* valuesModel = nullptr;
    PNSortFilterProxyModel* valuesProxyModel = nullptr;
    PNSqlQueryModel* filteredModel = nullptr;

    int selectedColumn = -1;  // nothing selected until construction

    QHash<QString, FilterSaveStructure> savedFilters;
};

#endif // FILTERDATADIALOG_H

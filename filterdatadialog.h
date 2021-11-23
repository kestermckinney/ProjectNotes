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
    QStringList ColumnValues;
    QString SearchString;
    QString SearchBeginValue;
    QString SearchEndValue;
};

class FilterDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDataDialog(QWidget *parent = nullptr);
    ~FilterDataDialog();

    void setFilterModel(PNSqlQueryModel* model);
    void setEndValue(QString& text);
    void setBeginValue(QString& text);
    void setSearchText(QString& text);
    QString getEndValue();
    QString getStartValue();
    QString getSearchText();

private slots:
    void on_lineEditSearchText_textEdited(const QString &arg1);

    void on_lineEditStartValue_textEdited(const QString &arg1);

    void on_lineEditEndValue_textEdited(const QString &arg1);

    void on_pushButtonApply_clicked();

    void on_pushButtonCancel_clicked();

    void on_pushButtonAll_clicked();

private:
    Ui::FilterDataDialog *ui;

    PNColumnModel* columnModel = nullptr;
    PNSortFilterProxyModel* columnProxyModel = nullptr;
    ValueSelectModel* valuesModel = nullptr;
    PNSortFilterProxyModel* valuesProxyModel = nullptr;
    PNSqlQueryModel* filteredModel = nullptr;

    QString filterName;

    int selectedColumn = -1;  // nothing selected until construction

    QHash<QString, FilterSaveStructure> savedFilters;
};

#endif // FILTERDATADIALOG_H

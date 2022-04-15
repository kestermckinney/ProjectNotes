#ifndef FILTERDATADIALOG_H
#define FILTERDATADIALOG_H

#include "FilterSaveStructure.h"
#include "pncolumnmodel.h"
#include "pnsqlquerymodel.h"

#include "valueselectmodel.h"
#include "pnsortfilterproxymodel.h"

#include <QDialog>

namespace Ui {
class FilterDataDialog;
}

class PNTableView;

class FilterDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDataDialog(QWidget *t_parent = nullptr);
    ~FilterDataDialog();

    void setSourceModelView(PNSqlQueryModel* t_model,PNTableView* t_view);
    void setEndValue(QVariant& t_text);
    void setBeginValue(QVariant& t_text);
    void setSearchText(QVariant& t_text);
    void setSearchTextEnabled( bool t_enabled );
    QVariant getEndValue();
    QVariant getStartValue();
    QVariant getSearchText();

private slots:
    void on_lineEditSearchText_textEdited(const QString &t_arg1);

    void on_lineEditStartValue_textEdited(const QString &t_arg1);

    void on_lineEditEndValue_textEdited(const QString &t_arg1);

    void on_pushButtonApply_clicked();

    void on_pushButtonCancel_clicked();

    void on_pushButtonAll_clicked();

    void on_pushButtonReset_clicked();

private:
    void setupFilters();
    Ui::FilterDataDialog *ui;

    PNColumnModel* m_column_model = nullptr;
    PNSortFilterProxyModel* m_column_proxy_model = nullptr;
    ValueSelectModel* m_values_model = nullptr;
    PNSortFilterProxyModel* m_values_proxy_model = nullptr;
    PNSqlQueryModel* m_filtered_model = nullptr;

    int m_selected_column = -1;  // nothing selected until construction

    QHash<QString, FilterSaveStructure> m_saved_filters;
};

#endif // FILTERDATADIALOG_H

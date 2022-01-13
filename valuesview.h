#ifndef VALUESVIEW_H
#define VALUESVIEW_H

#include "filterdatadialog.h"
#include "pntableview.h"
#include <QObject>

class ValuesView : public PNTableView
{
public:
    ValuesView(QWidget* t_parent = nullptr);
    ~ValuesView();
    void setSavedFilters( QHash<QString, FilterSaveStructure>* t_saved_filters ) {m_saved_filters = t_saved_filters; };

private:
    QHash<QString, FilterSaveStructure>* m_saved_filters;
    bool eventFilter(QObject *t_watched, QEvent *t_event) override;


public slots:
    void dataRowSelected(const QModelIndex &t_index) override;
};

#endif // VALUESVIEW_H


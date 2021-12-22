#ifndef VALUESVIEW_H
#define VALUESVIEW_H

#include "filterdatadialog.h"
#include "pntableview.h"
#include <QObject>

class ValuesView : public PNTableView
{
public:
    ValuesView(QWidget* parent = nullptr);
    ~ValuesView();
    void setSavedFilters( QHash<QString, FilterSaveStructure>* savedfilters ) {m_SavedFilters = savedfilters; };

private:
    QHash<QString, FilterSaveStructure>* m_SavedFilters;
    bool eventFilter(QObject *watched, QEvent *event) override;


public slots:
    void dataRowSelected(const QModelIndex &index) override;
};

#endif // VALUESVIEW_H


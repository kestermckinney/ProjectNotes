#ifndef SEARCHRESULTSVIEW_H
#define SEARCHRESULTSVIEW_H

#include "pntableview.h"

#include <QObject>

class SearchResultsView : public PNTableView
{
public:

    SearchResultsView(QWidget* t_parent = nullptr);
    ~SearchResultsView();

    void setModel(QAbstractItemModel *t_model) override;

private:

};

#endif // SEARCHRESULTSVIEW_H

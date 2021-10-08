#ifndef PNSORTFILTERPROXYMODEL_H
#define PNSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QObject>

class PNSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    PNSortFilterProxyModel(QObject* parent = 0);
    bool filterAcceptsRow(int source_row,
                          const QModelIndex &source_parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
};

#endif // PNSORTFILTERPROXYMODEL_H

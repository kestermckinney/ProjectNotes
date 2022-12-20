#ifndef ITEMDETAILSPAGE_H
#define ITEMDETAILSPAGE_H

#include "pnbasepage.h"
#include <QObject>
#include <QDataWidgetMapper>
#include "itemdetailsdelegate.h"

class ItemDetailsPage : public PNBasePage
{
    Q_OBJECT

public:
    ItemDetailsPage();
    ~ItemDetailsPage();

    void newRecord() override;

    void setupModels( Ui::MainWindow *t_ui ) override;

public slots:
    void toFirst();
    void toFirst( const QModelIndex& topLeft, const QModelIndex& bottomRight) { Q_UNUSED(topLeft);Q_UNUSED(bottomRight) toFirst(); }

private:
    Ui::MainWindow *ui = nullptr;

    QDataWidgetMapper* m_mapperItemDetails = nullptr;

    ItemDetailsDelegate* m_item_details_delegate = nullptr;
};

#endif // ITEMDETAILSPAGE_H

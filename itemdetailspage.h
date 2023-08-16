// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

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
    void toFirst(bool t_open = true) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;

private:
    Ui::MainWindow *ui = nullptr;

    QDataWidgetMapper* m_mapperItemDetails = nullptr;

    ItemDetailsDelegate* m_item_details_delegate = nullptr;
};

#endif // ITEMDETAILSPAGE_H

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

    void openRecord(QVariant& t_record_id) override;
    void newRecord() override;
    void setupModels( Ui::MainWindow *t_ui ) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;
    void submitRecord() override {if (m_mapperItemDetails) m_mapperItemDetails->submit(); }

private:
    Ui::MainWindow *ui = nullptr;

    QDataWidgetMapper* m_mapperItemDetails = nullptr;

    ItemDetailsDelegate* m_item_details_delegate = nullptr;
};

#endif // ITEMDETAILSPAGE_H

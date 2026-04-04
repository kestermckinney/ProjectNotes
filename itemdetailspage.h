// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ITEMDETAILSPAGE_H
#define ITEMDETAILSPAGE_H

#include "basepage.h"
#include <QObject>
#include <QDataWidgetMapper>
#include "itemdetailsdelegate.h"

class ItemDetailsPage : public BasePage
{
    Q_OBJECT

public:
    ItemDetailsPage();
    ~ItemDetailsPage();

    void openRecord(QVariant& recordId) override;
    void newRecord() override;
    void setupModels( Ui::MainWindow *ui ) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;
    void submitRecord() override {if (m_mapperItemDetails) m_mapperItemDetails->submit(); }

private:
    Ui::MainWindow *ui = nullptr;

    QDataWidgetMapper* m_mapperItemDetails = nullptr;

    ItemDetailsDelegate* m_itemDetailsDelegate = nullptr;
};

#endif // ITEMDETAILSPAGE_H

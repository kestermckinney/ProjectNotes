// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PEOPLEPAGE_H
#define PEOPLEPAGE_H

#include "basepage.h"

class PeoplePage : public BasePage
{
public:
    PeoplePage();
    void openRecord(QVariant &recordId) override;
    void setPageTitle() override;
    void buildPluginMenu(PluginManager* pm, QMenu* menu) override {};  // don't show a data integrated menu

    virtual void setupModels( Ui::MainWindow *ui ) override;
public slots:
    void setButtonAndMenuStates() override;

private:
    Ui::MainWindow *ui;
};

#endif // PEOPLEPAGE_H

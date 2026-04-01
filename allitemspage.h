// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ALLITEMSPAGE_H
#define ALLITEMSPAGE_H

#include "basepage.h"

class AllItemsPage : public BasePage
{
public:
    AllItemsPage();

    void setupModels(Ui::MainWindow *ui) override;
    void setPageTitle() override;
    void setButtonAndMenuStates() override;
    void buildPluginMenu(PluginManager* pm, QMenu* menu) override {};

private:
    Ui::MainWindow *ui = nullptr;
};

#endif // ALLITEMSPAGE_H

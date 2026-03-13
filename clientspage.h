// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CLIENTSPAGE_H
#define CLIENTSPAGE_H

#include "basepage.h"

class ClientsPage : public BasePage
{
public:
    ClientsPage();
    void openRecord(QVariant& recordId) override;
    virtual void setupModels( Ui::MainWindow *ui ) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;
    void buildPluginMenu(PluginManager* pm, QMenu* menu) override {};  // don't show a data integrated menu

private:
    Ui::MainWindow *ui;
};

#endif // CLIENTSPAGE_H

// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CLIENTSPAGE_H
#define CLIENTSPAGE_H

#include "pnbasepage.h"

class ClientsPage : public PNBasePage
{
public:
    ClientsPage();
    void openRecord(QVariant& t_record_id) override;
    virtual void setupModels( Ui::MainWindow *t_ui ) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;
    void buildPluginMenu(PluginManager* t_pm, QMenu* t_menu) override {};  // don't show a data integrated menu

private:
    Ui::MainWindow *ui;
};

#endif // CLIENTSPAGE_H

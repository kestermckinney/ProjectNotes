// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SEARCHPAGE_H
#define SEARCHPAGE_H

#include "pnbasepage.h"

class SearchPage : public PNBasePage
{
public:
    SearchPage();

    void setupModels( Ui::MainWindow *t_ui ) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;
    void buildPluginMenu(PluginManager* t_pm, QMenu* t_menu) override {};  // don't show a data integrated menu

private:
    Ui::MainWindow *ui;


private slots:
    bool eventFilter(QObject *watched, QEvent *event) override;

};

#endif // SEARCHPAGE_H

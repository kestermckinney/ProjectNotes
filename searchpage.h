// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SEARCHPAGE_H
#define SEARCHPAGE_H

#include "basepage.h"

class SearchPage : public BasePage
{
public:
    SearchPage();

    void setupModels( Ui::MainWindow *ui ) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;
    void buildPluginMenu(PluginManager* pm, QMenu* menu) override {};  // don't show a data integrated menu

private:
    Ui::MainWindow *ui;


private slots:
    bool eventFilter(QObject *watched, QEvent *event) override;

};

#endif // SEARCHPAGE_H

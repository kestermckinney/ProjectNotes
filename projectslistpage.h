// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSLISTPAGE_H
#define PROJECTSLISTPAGE_H

#include "basepage.h"

class ProjectsListPage : public BasePage
{
public:
    ProjectsListPage();
    void setupModels( Ui::MainWindow *ui ) override;
    void setPageTitle() override;
    void setButtonAndMenuStates() override;
    void buildPluginMenu(PluginManager* pm, QMenu* menu) override {};  // don't show a data integrated menu

private:
    Ui::MainWindow *ui;
};

#endif // PROJECTSLISTPAGE_H

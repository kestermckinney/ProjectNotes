// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTSLISTPAGE_H
#define PROJECTSLISTPAGE_H

#include "pnbasepage.h"

class ProjectsListPage : public PNBasePage
{
public:
    ProjectsListPage();
    void setupModels( Ui::MainWindow *t_ui );    
    void setPageTitle();
    void setButtonAndMenuStates();

private:
    Ui::MainWindow *ui;
};

#endif // PROJECTSLISTPAGE_H

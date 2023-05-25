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

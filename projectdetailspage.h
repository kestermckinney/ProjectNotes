#ifndef PROJECTDETAILSPAGE_H
#define PROJECTDETAILSPAGE_H

#include "pnbasepage.h"
#include <QObject>
#include <QDataWidgetMapper>
#include "projectdetailsdelegate.h"
#include "pncomboboxdelegate.h"

class ProjectDetailsPage : public PNBasePage
{
    Q_OBJECT

public:
    ProjectDetailsPage();
    ~ProjectDetailsPage();

    void setupModels( Ui::MainWindow *t_ui );

    void toFirst();

private:
    Ui::MainWindow *ui;

    QDataWidgetMapper* m_mapperProjectDetails = nullptr;

    ProjectDetailsDelegate* m_project_details_delegate = nullptr;
};

#endif // PROJECTDETAILSPAGE_H

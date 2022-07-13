#ifndef PROJECTDETAILSPAGE_H
#define PROJECTDETAILSPAGE_H

#include "pnbasepage.h"
#include <QObject>
#include <QDataWidgetMapper>

class ProjectDetailsPage : public PNBasePage
{
    Q_OBJECT

public:
    ProjectDetailsPage();

    void setupModels( Ui::MainWindow *t_ui );

private:
    Ui::MainWindow *ui;

    QDataWidgetMapper m_mapperProjectDetails;
};

#endif // PROJECTDETAILSPAGE_H

#ifndef PROJECTDETAILSPAGE_H
#define PROJECTDETAILSPAGE_H

#include "pnbasepage.h"
#include <QObject>
#include <QDataWidgetMapper>
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

    PNComboBoxDelegate* m_project_clients_delegate = nullptr;
};

#endif // PROJECTDETAILSPAGE_H

#ifndef PROJECTDETAILSPAGE_H
#define PROJECTDETAILSPAGE_H

#include "pnbasepage.h"
#include <QObject>
#include <QDataWidgetMapper>
#include "projectdetailsdelegate.h"
//#include "pncomboboxdelegate.h"

class ProjectDetailsPage : public PNBasePage
{
    Q_OBJECT

public:
    ProjectDetailsPage();
    ~ProjectDetailsPage();

    void newRecord() override;

    void setupModels( Ui::MainWindow *t_ui ) override;

    void toFirst();

private:
    Ui::MainWindow *ui = nullptr;

    QDataWidgetMapper* m_mapperProjectDetails = nullptr;

    ProjectDetailsDelegate* m_project_details_delegate = nullptr;

private slots:
    void on_tabWidgetProject_currentChanged(int index);
};

#endif // PROJECTDETAILSPAGE_H

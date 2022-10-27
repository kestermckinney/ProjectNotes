#ifndef PROJECTDETAILSPAGE_H
#define PROJECTDETAILSPAGE_H

#include "pnbasepage.h"
#include <QObject>
#include <QDataWidgetMapper>
#include "projectdetailsdelegate.h"

#include <QList>

class ProjectDetailsPage : public PNBasePage
{
    Q_OBJECT

public:
    ProjectDetailsPage();
    ~ProjectDetailsPage();

    void newRecord() override;

    void setupModels( Ui::MainWindow *t_ui ) override;

public slots:
    void toFirst();
    void toFirst( const QModelIndex& topLeft, const QModelIndex& bottomRight) { Q_UNUSED(topLeft);Q_UNUSED(bottomRight) toFirst(); }

private:
    Ui::MainWindow *ui = nullptr;

    QDataWidgetMapper* m_mapperProjectDetails = nullptr;

    ProjectDetailsDelegate* m_project_details_delegate = nullptr;

private slots:
    void on_tabWidgetProject_currentChanged(int index);
};

#endif // PROJECTDETAILSPAGE_H

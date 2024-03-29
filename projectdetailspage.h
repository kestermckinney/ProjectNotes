// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

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
    void openRecord(QVariant& t_record_id) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;
    void submitRecord() override {if (m_mapperProjectDetails) m_mapperProjectDetails->submit(); }

private:
    Ui::MainWindow *ui = nullptr;

    QDataWidgetMapper* m_mapperProjectDetails = nullptr;

    ProjectDetailsDelegate* m_project_details_delegate = nullptr;

private slots:
    void on_tabWidgetProject_currentChanged(int index);

};

#endif // PROJECTDETAILSPAGE_H

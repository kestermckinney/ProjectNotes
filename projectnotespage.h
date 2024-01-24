// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTNOTESPAGE_H
#define PROJECTNOTESPAGE_H

#include "pnbasepage.h"
#include <QObject>
#include <QDataWidgetMapper>
#include "projectnotesdelegate.h"

class ProjectNotesPage : public PNBasePage
{
    Q_OBJECT

public:
    ProjectNotesPage();
    ~ProjectNotesPage();

    void newRecord() override;
    void setupModels( Ui::MainWindow *t_ui ) override;
    void openRecord(QVariant& t_record_id) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;

private:
    Ui::MainWindow *ui = nullptr;
    QDataWidgetMapper* m_mapperProjectNotes = nullptr;
    ProjectNotesDelegate* m_project_notes_delegate = nullptr;

private slots:
    void on_tabWidgetNotes_currentChanged(int index);
};

#endif // PROJECTNOTESPAGE_H

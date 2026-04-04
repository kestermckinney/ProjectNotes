// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTNOTESPAGE_H
#define PROJECTNOTESPAGE_H

#include "basepage.h"
#include <QObject>
#include <QDataWidgetMapper>
#include "projectnotesdelegate.h"

class ProjectNotesPage : public BasePage
{
    Q_OBJECT

public:
    ProjectNotesPage();
    ~ProjectNotesPage();

    void newRecord() override;
    void setupModels( Ui::MainWindow *ui ) override;
    void openRecord(QVariant& recordId) override;
    void setButtonAndMenuStates() override;
    void setPageTitle() override;
    void submitRecord() override {if (m_mapperProjectNotes) m_mapperProjectNotes->submit(); }

private:
    Ui::MainWindow *ui = nullptr;
    QDataWidgetMapper* m_mapperProjectNotes = nullptr;
    ProjectNotesDelegate* m_projectNotesDelegate = nullptr;

private slots:
    void on_tabWidgetNotes_currentChanged(int index);
};

#endif // PROJECTNOTESPAGE_H

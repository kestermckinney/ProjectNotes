// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTTEAMMEMBERSVIEW_H
#define PROJECTTEAMMEMBERSVIEW_H

#include "tableview.h"
#include "sqlcomboboxdelegate.h"
#include "checkboxdelegate.h"
#include "plaintexteditdelegate.h"
#include <QObject>

class ProjectTeamMembersView : public TableView
{
public:
    ProjectTeamMembersView(QWidget* parent = nullptr);
    ~ProjectTeamMembersView();

    void setModel(QAbstractItemModel *model) override;

private:
    // projects list panel delegates
    SqlComboBoxDelegate* m_unfilteredPeopleDelegate = nullptr;
    CheckBoxDelegate* m_receiveStatusDelegate = nullptr;
    PlainTextEditDelegate* m_roleDelegate = nullptr;

public slots:
    void slotNewRecord() override;
};

#endif // PROJECTTEAMMEMBERSVIEW_H

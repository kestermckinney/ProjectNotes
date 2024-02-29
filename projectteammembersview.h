// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTTEAMMEMBERSVIEW_H
#define PROJECTTEAMMEMBERSVIEW_H

#include "pntableview.h"
#include "pncomboboxdelegate.h"
#include "pncheckboxdelegate.h"
#include "pnplaintexteditdelegate.h"
#include <QObject>

class ProjectTeamMembersView : public PNTableView
{
public:
    ProjectTeamMembersView(QWidget* t_parent = nullptr);
    ~ProjectTeamMembersView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    // projects list panel delegates
    PNComboBoxDelegate* m_unfiltered_people_delegate = nullptr;
    PNCheckBoxDelegate* m_receive_status_delegate = nullptr;
    PNPlainTextEditDelegate* m_role_delegate = nullptr;

public slots:
    void slotNewRecord() override;
};

#endif // PROJECTTEAMMEMBERSVIEW_H

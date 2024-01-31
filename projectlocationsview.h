// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTLOCATIONSVIEW_H
#define PROJECTLOCATIONSVIEW_H

#include "pntableview.h"
#include "pnlineeditfilebuttondelegate.h"
#include "pnplaintexteditdelegate.h"
#include "comboboxdelegate.h"
#include <QObject>
#include <QStringListModel>

class ProjectLocationsView : public PNTableView
{
public:
    ProjectLocationsView(QWidget* t_parent = nullptr);
    ~ProjectLocationsView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    QStringListModel m_file_types;//(PNDatabaseObjects::file_types);

    // projects list panel delegates
    ComboBoxDelegate* m_file_type_delegate = nullptr;
    PNLineEditFileButtonDelegate* m_file_button_delegate = nullptr;
    PNPlainTextEditDelegate* m_description_delegate = nullptr;
};


#endif // PROJECTLOCATIONSVIEW_H

// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTNOTESVIEW_H
#define PROJECTNOTESVIEW_H

#include "tableview.h"
#include "dateeditdelegate.h"
#include "checkboxdelegate.h"
#include "plaintexteditdelegate.h"
#include <QObject>


class ProjectNotesView : public TableView
{
public:
    ProjectNotesView(QWidget* parent = nullptr);
    ~ProjectNotesView();

    void setModel(QAbstractItemModel *model) override;

private:
    // projects list panel delegates
    DateEditDelegate* m_meetingDateDelegate = nullptr;
    CheckBoxDelegate* m_internalItemDelegate = nullptr;
    PlainTextEditDelegate* m_titleDelegate = nullptr;

public slots:
    void slotNewRecord() override;
};


#endif // PROJECTNOTESVIEW_H

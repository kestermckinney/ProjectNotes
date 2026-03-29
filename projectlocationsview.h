// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTLOCATIONSVIEW_H
#define PROJECTLOCATIONSVIEW_H

#include "tableview.h"
#include "lineeditfilebuttondelegate.h"
#include "plaintexteditdelegate.h"
#include "comboboxdelegate.h"
#include <QObject>
#include <QStringListModel>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

class ProjectLocationsView : public TableView
{
public:
    ProjectLocationsView(QWidget* parent = nullptr);
    ~ProjectLocationsView();

    void setModel(QAbstractItemModel *model) override;

private:
    QStringListModel m_fileTypes;//(DatabaseObjects::file_types);

    // projects list panel delegates
    ComboBoxDelegate* m_fileTypeDelegate = nullptr;
    LineEditFileButtonDelegate* m_fileButtonDelegate = nullptr;
    PlainTextEditDelegate* m_descriptionDelegate = nullptr;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public slots:
    void slotNewRecord() override;
};


#endif // PROJECTLOCATIONSVIEW_H

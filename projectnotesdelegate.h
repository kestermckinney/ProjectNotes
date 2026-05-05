// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTNOTESDELEGATE_H
#define PROJECTNOTESDELEGATE_H

#include <QStyledItemDelegate>
#include <QObject>

class MainWindow;

class ProjectNotesDelegate : public QStyledItemDelegate
{
public:
    explicit ProjectNotesDelegate(QObject *parent = nullptr);
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    static MainWindow* findMainWindow();

    QString getScaledHtmlForWidget(const QString& html) const;
    QString getUncaledHtmlForSave(const QString& html) const;

    void setMainWindow(MainWindow* mainWin) { m_mainWindow = mainWin; }

    mutable MainWindow* m_mainWindow = nullptr;
};

#endif // PROJECTNOTESDELEGATE_H

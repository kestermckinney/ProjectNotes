// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LINEEDITFILEBUTTON_H
#define LINEEDITFILEBUTTON_H

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class LineEditFileButton : public QWidget
{

    Q_OBJECT

public:
    LineEditFileButton(QWidget *parent );
    ~LineEditFileButton();

    QString fileName() const;

public slots:
    void setFileName( const QString &fn );

private slots:
    void chooseFile();

signals:
    void fileNameChanged( const QString& );

private:
    QPushButton* m_pushButton;
    QLineEdit* m_lineEdit;
};

#endif // LINEEDITFILEBUTTON_H

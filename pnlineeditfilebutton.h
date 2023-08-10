// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNLINEEDITFILEBUTTON_H
#define PNLINEEDITFILEBUTTON_H

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class PNLineEditFileButton : public QWidget
{

    Q_OBJECT

public:
    PNLineEditFileButton(QWidget *t_parent );
    ~PNLineEditFileButton();

    QString fileName() const;

public slots:
    void setFileName( const QString &fn );

private slots:
    void chooseFile();

signals:
    void fileNameChanged( const QString& );

private:
    QPushButton* m_push_button;
    QLineEdit* m_line_edit;
};

#endif // PNLINEEDITFILEBUTTON_H

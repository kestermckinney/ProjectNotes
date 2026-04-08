// Copyright (C) 2026 Paul McKinney
#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QComboBox>
#include <QObject>
#include <QWidget>

class ComboBox : public QComboBox
{
    Q_OBJECT

public:
    ComboBox(QWidget *parent = nullptr);

protected:
    /*! \reimp */ void wheelEvent(QWheelEvent *event) override;
    /*! \reimp */ void childEvent(QChildEvent *event) override;
    /*! \reimp */ void showEvent(QShowEvent *event) override;

private:
    void updateBackground();
};

#endif // COMBOBOX_H

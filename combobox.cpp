// Copyright (C) 2026 Paul McKinney
#include "combobox.h"
#include <QWheelEvent>
#include <QChildEvent>
#include <QShowEvent>
#include <QLineEdit>
#include <QApplication>

ComboBox::ComboBox(QWidget *parent) : QComboBox(parent)
{

}

void ComboBox::wheelEvent(QWheelEvent *event)
{
    event->ignore();  // this helps to avoid accidental value chages
}

void ComboBox::childEvent(QChildEvent *event)
{
    QComboBox::childEvent(event);
    if (qobject_cast<QLineEdit*>(event->child()))
        updateBackground();
}

void ComboBox::showEvent(QShowEvent *event)
{
    QComboBox::showEvent(event);
    updateBackground();
}

void ComboBox::updateBackground()
{
    QPalette pal = QApplication::palette();
    if (!isEditable())
    {
        QColor c = pal.color(QPalette::Button);
        pal.setColor(QPalette::Base, c);
        pal.setColor(QPalette::Button, c);
        setPalette(pal);
    }
    else
    {
        setPalette(pal);
    }
}

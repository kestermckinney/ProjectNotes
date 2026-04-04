// Copyright (C) 2026 Paul McKinney
#include "combobox.h"
#include <QWheelEvent>

ComboBox::ComboBox(QWidget *parent) : QComboBox(parent)
{

}

void ComboBox::wheelEvent(QWheelEvent *event)
{
    event->ignore();  // this helps to avoid accidental value chages
}

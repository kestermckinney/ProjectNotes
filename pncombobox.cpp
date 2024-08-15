#include "pncombobox.h"
#include <QWheelEvent>

PNComboBox::PNComboBox(QWidget *t_parent) : QComboBox(t_parent)
{

}

void PNComboBox::wheelEvent(QWheelEvent *t_event)
{
    t_event->ignore();  // this helps to avoid accidental value chages
}

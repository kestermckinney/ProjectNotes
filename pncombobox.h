#ifndef PNCOMBOBOX_H
#define PNCOMBOBOX_H

#include <QComboBox>
#include <QObject>
#include <QWidget>

class PNComboBox : public QComboBox
{
    Q_OBJECT

public:
    PNComboBox(QWidget *t_parent = nullptr);

protected:
    /*! \reimp */ void wheelEvent(QWheelEvent *event) override;
};

#endif // PNCOMBOBOX_H

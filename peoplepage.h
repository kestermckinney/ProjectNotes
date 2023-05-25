#ifndef PEOPLEPAGE_H
#define PEOPLEPAGE_H

#include "pnbasepage.h"

class PeoplePage : public PNBasePage
{
public:
    PeoplePage();

    virtual void setupModels( Ui::MainWindow *t_ui );
public slots:
    void setButtonAndMenuStates();

private:
    Ui::MainWindow *ui;
};

#endif // PEOPLEPAGE_H

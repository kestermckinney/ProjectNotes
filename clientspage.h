#ifndef CLIENTSPAGE_H
#define CLIENTSPAGE_H

#include "pnbasepage.h"

class ClientsPage : public PNBasePage
{
public:
    ClientsPage();

    virtual void setupModels( Ui::MainWindow *t_ui );
    void copyItem();

private:
    Ui::MainWindow *ui;
};

#endif // CLIENTSPAGE_H

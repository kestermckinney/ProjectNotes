#ifndef HELPPAGE_H
#define HELPPAGE_H

#include "pnbasepage.h"

class HelpPage : public PNBasePage
{

public:
    HelpPage();

    void search();
    virtual void setupModels( Ui::MainWindow *t_ui );
    void setPageTitle();
    void setButtonAndMenuStates();

public slots:
    void showLink(const QUrl &url);

private:
    Ui::MainWindow *ui;
};

#endif // HELPPAGE_H

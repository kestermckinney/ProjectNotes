#ifndef SEARCHPAGE_H
#define SEARCHPAGE_H

#include "pnbasepage.h"

class SearchPage : public PNBasePage
{
public:
    SearchPage();

    void setupModels( Ui::MainWindow *t_ui );

private:
    Ui::MainWindow *ui;

private slots:

};

#endif // SEARCHPAGE_H

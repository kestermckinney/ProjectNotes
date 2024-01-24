// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef HELPPAGE_H
#define HELPPAGE_H

#include "pnbasepage.h"

class HelpPage : public PNBasePage
{

public:
    HelpPage();

    void search();
    virtual void setupModels( Ui::MainWindow *t_ui );
    void setPageTitle() override;
    void setButtonAndMenuStates();

public slots:
    void showLink(const QUrl &url);

private:
    Ui::MainWindow *ui;
};

#endif // HELPPAGE_H

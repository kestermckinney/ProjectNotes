// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PEOPLEPAGE_H
#define PEOPLEPAGE_H

#include "pnbasepage.h"

class PeoplePage : public PNBasePage
{
public:
    PeoplePage();
    void openRecord(QVariant &t_record_id) override;
    void setPageTitle() override;
    virtual void setupModels( Ui::MainWindow *t_ui ) override;
public slots:
    void setButtonAndMenuStates() override;

private:
    Ui::MainWindow *ui;
};

#endif // PEOPLEPAGE_H

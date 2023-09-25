// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DATABASESTRUCTURE_H
#define DATABASESTRUCTURE_H

#include <QString>

class DatabaseStructure
{
public:
    bool CreateDatabase();
    bool UpgradeDatabase();
};

#endif // DATABASESTRUCTURE_H

// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef FILTERSAVESTRUCTURE_H
#define FILTERSAVESTRUCTURE_H
#include <QVariant>

class FilterSaveStructure
{
public:
    QVariantList ColumnValues;
    QVariant SearchString;
    QVariant SearchBeginValue;
    QVariant SearchEndValue;
};


#endif // FILTERSAVESTRUCTURE_H

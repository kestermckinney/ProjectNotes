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

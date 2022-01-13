#ifndef FILTERSAVESTRUCTURE_H
#define FILTERSAVESTRUCTURE_H
#include <QVariant>

class FilterSaveStructure
{
public:
    QVariantList ColumnValues;
    QVariant SearchString;
    QVariant t_search_begin_value;
    QVariant t_search_end_value;
};


#endif // FILTERSAVESTRUCTURE_H

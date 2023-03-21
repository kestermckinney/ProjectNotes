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

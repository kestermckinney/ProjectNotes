#ifndef CLIENTSMODEL_H
#define CLIENTSMODEL_H

#include "pnsqlquerymodel.h"

class ClientsModel : public PNSqlQueryModel
{
public:
    ClientsModel(QObject* t_parent);
    bool newRecord() override;
};

#endif // CLIENTSMODEL_H

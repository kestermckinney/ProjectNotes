#ifndef CLIENTSMODEL_H
#define CLIENTSMODEL_H

#include "pnsqlquerymodel.h"

class ClientsModel : public PNSqlQueryModel
{
public:
    ClientsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new ClientsModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // CLIENTSMODEL_H

#ifndef NOTESACTIONITEMSMODEL_H
#define NOTESACTIONITEMSMODEL_H

#include "pnsqlquerymodel.h"

class NotesActionItemsModel : public PNSqlQueryModel
{
public:
    NotesActionItemsModel(QObject* t_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new NotesActionItemsModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // NOTESACTIONITEMSMODEL_H

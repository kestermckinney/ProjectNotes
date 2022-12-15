#ifndef MEETINGATTENDEESMODEL_H
#define MEETINGATTENDEESMODEL_H

#include "pnsqlquerymodel.h"

class MeetingAttendeesModel : public PNSqlQueryModel
{
public:
    MeetingAttendeesModel(QObject* m_parent);
    PNSqlQueryModel* createExportVersion() override { return dynamic_cast<PNSqlQueryModel*>(new MeetingAttendeesModel(this)); };
    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // MEETINGATTENDEESMODEL_H

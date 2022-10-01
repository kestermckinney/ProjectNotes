#ifndef MEETINGATTENDEESMODEL_H
#define MEETINGATTENDEESMODEL_H

#include "pnsqlquerymodel.h"

class MeetingAttendeesModel : public PNSqlQueryModel
{
public:
    MeetingAttendeesModel(QObject* m_parent);

    bool newRecord(const QVariant* t_fk_value1 = nullptr, const QVariant* t_fk_value2 = nullptr) override;
};

#endif // MEETINGATTENDEESMODEL_H

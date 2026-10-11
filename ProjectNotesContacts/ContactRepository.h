// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ContactTypes.h"
#include <QSqlDatabase>
class DatabaseObjects;
namespace PN::Contacts {
class ContactRepository {
public:
    explicit ContactRepository(DatabaseObjects *database):m_database(database) {}
    ContactRepository() = default;
    ~ContactRepository() { close(); }
    bool open(const QString &path, QString &error);
    void close();
    bool isOpen() const { return m_connection.isOpen(); }
    std::function<void()> onChanged;
    bool read(QList<Person> &people,QList<Client> &clients,QString &error) const;
    bool createClient(const QString &id,const QString &name,QString &error);
    bool updateClientName(const QString &id,const QString &expectedName,const QString &name,QString &error);
    // expected is an optimistic snapshot: a stale match cannot overwrite a later edit.
    bool apply(const Contact &contact,const QString &personId,const QString &clientId,
               const QString &newClientId,const QString &displayName,const QVariantMap &expected,QString &error,
               bool reviewed=false,bool linked=false);
private:
    QSqlDatabase database() const;
    DatabaseObjects *m_database = nullptr;
    QSqlDatabase m_connection;
    QString m_connectionName;
};
}

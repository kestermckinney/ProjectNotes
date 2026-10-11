// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QJsonObject>
#include <QStringList>
#include <QVariantMap>
#include <functional>

namespace PN::Contacts {
struct Contact {
    QString id, book, name, company;
    QString etag;
    QStringList emails, phones;
    // Presence distinguishes an omitted field from an explicitly empty field.
    QVariantMap fields;
};
struct Person {
    QString id, name, clientId, company;
    QVariantMap fields;
    bool deleted = false;
};
struct Client { QString id, name; bool deleted = false; };
struct Candidate { QString id, label, reason; double score = 0; bool strong = false; };
struct Match { QString id; QList<Candidate> candidates; bool review = false; };
struct Snapshot {
    QString account;
    QList<Contact> contacts;
    QVariantList books;
    QString defaultExportBookId;
    QJsonObject cursors;
    bool complete = true;
    QString error;
    bool authenticationError = false;
};
QJsonObject encode(const Contact &contact);
Contact decode(const QJsonObject &object);
QString sourceKey(const QString &account, const Contact &contact);
QString exportBook(const QVariantList &books, const QString &preferred = {}, bool defaultOnly = false);
}

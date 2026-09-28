// SPDX-License-Identifier: GPL-3.0-only
#include "ContactTypes.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
namespace PN::Contacts {
QString exportBook(const QVariantList &books, const QString &preferred, bool defaultOnly) {
    QString fallback;
    for (const auto &value : books) {
        const auto book = value.toMap();
        const auto id = book.value("id").toString();
        if (!book.value("writable").toBool() || id.isEmpty()) continue;
        if (id == preferred) return id;
        if (fallback.isEmpty() || id < fallback) fallback = id;
    }
    return defaultOnly ? QString() : fallback;
}
QJsonObject encode(const Contact &c) {
    return {{"id", c.id}, {"book", c.book}, {"name", c.name}, {"company", c.company}, {"etag",c.etag},
            {"emails", QJsonArray::fromStringList(c.emails)}, {"phones", QJsonArray::fromStringList(c.phones)},
            {"fields", QJsonObject::fromVariantMap(c.fields)}};
}
Contact decode(const QJsonObject &o) {
    Contact c; c.id=o["id"].toString(); c.book=o["book"].toString();
    c.name=o["name"].toString(); c.company=o["company"].toString();
    c.etag=o["etag"].toString();
    for (const auto &v:o["emails"].toArray()) c.emails.append(v.toString());
    for (const auto &v:o["phones"].toArray()) c.phones.append(v.toString());
    c.fields=o["fields"].toObject().toVariantMap(); return c;
}
QString sourceKey(const QString &account, const Contact &c) {
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonArray{account,c.book,c.id}).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
}

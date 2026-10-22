// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "personvcard.h"

#include <QPair>
#include <QSqlQuery>
#include <QStringList>

static QString escaped(QString s)
{
    s.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    s.replace(QStringLiteral("\r\n"), QStringLiteral("\\n"));
    s.replace(u'\n', QStringLiteral("\\n"));
    s.remove(u'\r');
    s.replace(QStringLiteral(";"), QStringLiteral("\\;"));
    s.replace(QStringLiteral(","), QStringLiteral("\\,"));
    return s;
}

QByteArray writePersonVCard(const QVariantMap& fields, const QString& uid, const QString& prodId)
{
    const QString name = fields.value(QStringLiteral("name")).toString();
    QStringList lines = {QStringLiteral("BEGIN:VCARD"), QStringLiteral("VERSION:3.0")};
    if (!uid.isEmpty())
        lines << QStringLiteral("UID:") + escaped(uid);
    lines << QStringLiteral("FN:") + escaped(name) << QStringLiteral("N:;") + escaped(name) + QStringLiteral(";;;");
    const QList<QPair<QString, QString>> properties = {
        {QStringLiteral("ORG"), QStringLiteral("company")},
        {QStringLiteral("TITLE"), QStringLiteral("role")},
        {QStringLiteral("EMAIL;TYPE=WORK"), QStringLiteral("email")},
        {QStringLiteral("TEL;TYPE=WORK"), QStringLiteral("office_phone")},
        {QStringLiteral("TEL;TYPE=CELL"), QStringLiteral("cell_phone")}};
    for (const auto& property : properties) {
        const QString value = fields.value(property.second).toString().trimmed();
        if (!value.isEmpty())
            lines << property.first + u':' + escaped(value);
    }
    if (!prodId.isEmpty())
        lines << QStringLiteral("PRODID:") + prodId;
    lines << QStringLiteral("END:VCARD");

    QByteArray out;
    for (const QString& line : lines) {
        QByteArray bytes = line.toUtf8();
        while (bytes.size() > 75) {
            int n = 75;
            while (n > 0 && (uchar(bytes[n]) & 0xc0) == 0x80)
                --n;
            out += bytes.left(n) + "\r\n ";
            bytes = bytes.mid(n);
        }
        out += bytes + "\r\n";
    }
    return out;
}

QByteArray personVCard(QSqlDatabase& db, const QString& personId, QString* name)
{
    if (personId.isEmpty())
        return {};
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT p.name, p.email, p.office_phone, p.cell_phone, p.role, c.client_name "
        "FROM people p LEFT JOIN clients c ON c.id = p.client_id AND c.deleted = 0 "
        "WHERE p.id = ? AND p.deleted = 0"));
    query.addBindValue(personId);
    if (!query.exec() || !query.next())
        return {};
    const QVariantMap fields {
        {QStringLiteral("name"), query.value(0)},
        {QStringLiteral("email"), query.value(1)},
        {QStringLiteral("office_phone"), query.value(2)},
        {QStringLiteral("cell_phone"), query.value(3)},
        {QStringLiteral("role"), query.value(4)},
        {QStringLiteral("company"), query.value(5)}};
    if (name)
        *name = query.value(0).toString();
    return writePersonVCard(fields, personId, QStringLiteral("-//Project Notes//Contact Card//EN"));
}

QString personVCardFileName(const QString& name)
{
    QString stem = name.simplified();
    for (QChar& character : stem)
        if (QStringLiteral("/\\:*?\"<>|").contains(character) || character.unicode() < 0x20)
            character = u'_';
    while (stem.startsWith(u'.'))
        stem.remove(0, 1);
    if (stem.isEmpty())
        stem = QStringLiteral("Contact");
    return stem + QStringLiteral(".vcf");
}

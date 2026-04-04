// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VCARDPARSER_H
#define VCARDPARSER_H

#include "databaseobjects.h"

#include <QFile>
#include <QList>
#include <QMimeData>
#include <QRegularExpression>
#include <QString>
#include <QUrl>
#include <QVector>

struct VCardContact
{
    QString name;
    QString company;
    QString email;
    QString officePhone;
    QString cellPhone;
    QString role;
};

// Unfold vCard line continuations (RFC 6350 §3.2)
inline QString unfoldVCard(const QString& text)
{
    QString result = text;
    result.replace(QRegularExpression("\r\n[ \t]"), "");
    result.replace(QRegularExpression("\n[ \t]"), "");
    return result;
}

// Unescape vCard field values (RFC 6350 §3.4):
//   \\ -> \    \, -> ,    \; -> ;    \n/\N -> newline
inline QString unescapeVCardValue(const QString& value)
{
    QString result;
    result.reserve(value.size());
    bool escaped = false;
    for (const QChar& ch : value)
    {
        if (escaped)
        {
            switch (ch.toLatin1())
            {
            case 'n': case 'N': result += ' '; break; // newlines in field values become spaces
            case '\\': result += '\\'; break;
            case ',':  result += ',';  break;
            case ';':  result += ';';  break;
            case ':':  result += ':';  break;
            default:   result += ch;   break;
            }
            escaped = false;
        }
        else if (ch == '\\')
        {
            escaped = true;
        }
        else
        {
            result += ch;
        }
    }
    return result.trimmed();
}

// Parse all vCards found in text; returns one entry per BEGIN:VCARD block
inline QList<VCardContact> parseVCards(const QString& rawText)
{
    QList<VCardContact> contacts;
    QString text = unfoldVCard(rawText);

    QRegularExpression blockRe("BEGIN:VCARD(.*?)END:VCARD",
                               QRegularExpression::DotMatchesEverythingOption |
                               QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = blockRe.globalMatch(text);

    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        QString block = match.captured(1);
        QStringList lines = block.split(QRegularExpression("\r\n|\n|\r"));

        VCardContact contact;

        for (const QString& rawLine : lines)
        {
            QString line = rawLine.trimmed();
            if (line.isEmpty()) continue;

            int colonPos = line.indexOf(':');
            if (colonPos < 0) continue;

            QString prop  = line.left(colonPos).toUpper();
            QString value = unescapeVCardValue(line.mid(colonPos + 1));
            if (value.isEmpty()) continue;

            if (prop == "FN")
            {
                contact.name = value;
            }
            else if (prop == "N" && contact.name.isEmpty())
            {
                // N:Last;First;Middle;Prefix;Suffix
                QStringList parts = value.split(';');
                QString last  = parts.value(0).trimmed();
                QString first = parts.value(1).trimmed();
                if (!first.isEmpty() && !last.isEmpty())
                    contact.name = first + " " + last;
                else
                    contact.name = last.isEmpty() ? first : last;
            }
            else if (prop == "ORG" || prop.startsWith("ORG;"))
            {
                // ORG:Company;Department — take just the company part
                contact.company = value.split(';').first().trimmed();
            }
            else if (prop == "TITLE" || prop.startsWith("TITLE;"))
            {
                contact.role = value;
            }
            else if (prop.startsWith("EMAIL"))
            {
                if (contact.email.isEmpty())
                    contact.email = value;
            }
            else if (prop.startsWith("TEL"))
            {
                bool isCell = prop.contains("CELL",   Qt::CaseInsensitive) ||
                              prop.contains("MOBILE", Qt::CaseInsensitive);
                bool isWork = prop.contains("WORK",   Qt::CaseInsensitive);
                bool isHome = prop.contains("HOME",   Qt::CaseInsensitive);

                if (isCell && contact.cellPhone.isEmpty())
                    contact.cellPhone = value;
                else if ((isWork || (!isCell && !isHome)) && contact.officePhone.isEmpty())
                    contact.officePhone = value;
            }
        }

        if (!contact.name.isEmpty())
            contacts.append(contact);
    }

    return contacts;
}

// Try to read a local file from a URL. Returns empty string if not readable.
inline QString readLocalFile(const QUrl& url)
{
    // toLocalFile() returns empty for non-local URLs, so no need for isLocalFile()
    QString path = url.toLocalFile();
    if (path.isEmpty())
        return QString();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    return QString::fromUtf8(file.readAll());
}

// Lightweight check for dragEnter/dragMove — does NOT read file contents.
// Accepts if MIME data contains vCard type OR if any dropped file has a .vcf extension.
inline bool mimeDataHasVCard(const QMimeData* mimeData)
{
    // Check all available MIME format names for anything vCard-related
    for (const QString& fmt : mimeData->formats())
    {
        if (fmt.contains("vcard", Qt::CaseInsensitive))
            return true;
    }

    // Check plain text content
    if (mimeData->hasText() &&
        mimeData->text().contains("BEGIN:VCARD", Qt::CaseInsensitive))
        return true;

    // Check for dropped .vcf files
    if (mimeData->hasUrls())
    {
        for (const QUrl& url : mimeData->urls())
        {
            QString path = url.toLocalFile();
            if (!path.isEmpty() && path.endsWith(".vcf", Qt::CaseInsensitive))
                return true;
        }
    }

    return false;
}

// Extract raw vCard text from drop MIME data.
// Path 1 — direct vCard MIME type (Outlook drag, contact app drag)
// Path 2 — local file drop (.vcf or any file whose content begins with BEGIN:VCARD)
inline QString extractVCardText(const QMimeData* mimeData)
{
    // Path 1: direct vCard MIME type data
    for (const QString& fmt : mimeData->formats())
    {
        if (fmt.contains("vcard", Qt::CaseInsensitive))
        {
            QString text = QString::fromUtf8(mimeData->data(fmt));
            if (text.contains("BEGIN:VCARD", Qt::CaseInsensitive))
                return text;
        }
    }

    // Path 1b: plain text that is vCard data
    if (mimeData->hasText())
    {
        QString text = mimeData->text();
        if (text.contains("BEGIN:VCARD", Qt::CaseInsensitive))
            return text;
    }

    // Path 2: file drop — read each local file and check for vCard content
    if (mimeData->hasUrls())
    {
        QString combined;
        for (const QUrl& url : mimeData->urls())
        {
            QString content = readLocalFile(url);
            if (content.contains("BEGIN:VCARD", Qt::CaseInsensitive))
                combined += content + "\n";
        }
        if (!combined.isEmpty())
            return combined;
    }

    return QString();
}

// Returns the client_id for companyName, creating the client record if it doesn't exist.
// Returns an empty string if companyName is empty.
inline QString findOrCreateClient(DatabaseObjects* dbo, const QString& companyName)
{
    if (companyName.isEmpty())
        return QString();

    QString escaped = companyName;
    escaped.replace("'", "''");

    QString clientId = dbo->execute(
        QString("SELECT id FROM clients WHERE client_name = '%1' AND deleted = 0").arg(escaped));

    if (!clientId.isEmpty())
        return clientId;

    ClientsModel* cm = dbo->clientsmodel();
    QVector<QVariant> qr = cm->emptyrecord();
    qr[1] = companyName;
    QModelIndex idx = cm->addRecord(qr);
    if (!idx.isValid())
        return QString();

    if (!cm->insertCacheRow(idx.row()))
        return QString();

    return cm->data(cm->index(idx.row(), 0)).toString();
}

// Returns the people_id for the contact, creating the person record if it doesn't exist.
inline QString findOrCreatePerson(DatabaseObjects* dbo, const VCardContact& contact, const QString& clientId)
{
    QString escapedName = contact.name;
    escapedName.replace("'", "''");

    QString personId = dbo->execute(
        QString("SELECT id FROM people WHERE name = '%1' AND deleted = 0").arg(escapedName));

    if (!personId.isEmpty())
        return personId;

    PeopleModel* pm = dbo->unfilteredpeoplemodel();
    QVector<QVariant> qr = pm->emptyrecord();
    qr[1] = contact.name;
    qr[2] = contact.email;
    qr[3] = contact.officePhone;
    qr[4] = contact.cellPhone;
    if (!clientId.isEmpty()) qr[5] = clientId;
    qr[6] = contact.role;

    QModelIndex idx = pm->addRecord(qr);
    if (!idx.isValid())
        return QString();

    if (!pm->insertCacheRow(idx.row()))
        return QString();

    return pm->data(pm->index(idx.row(), 0)).toString();
}

#endif // VCARDPARSER_H

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>
#include <QSqlDatabase>
#include <QString>
#include <QVariantMap>

// vCard 3.0 (RFC 2426) output for a contact in the people table, shared by the
// desktop and mobile apps' Export / Email Contact Card actions and by the
// plugin API's contacts.toVCard.

// Writes one card from these keys: name, company, role, email, office_phone,
// cell_phone. Empty values are left out. Lines are CRLF-terminated and folded
// at 75 octets without splitting a UTF-8 sequence.
QByteArray writePersonVCard(const QVariantMap& fields, const QString& uid, const QString& prodId);

// Looks up a people row (and its company name) and writes its card. Returns an
// empty array when the person doesn't exist or has been deleted. When name is
// non-null it receives the contact's name.
QByteArray personVCard(QSqlDatabase& db, const QString& personId, QString* name = nullptr);

// A file name for the card, "<name>.vcf", with characters that aren't allowed
// in file names on Windows, macOS or Linux replaced.
QString personVCardFileName(const QString& name);

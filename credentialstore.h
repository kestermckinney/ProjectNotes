// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <QString>

class CredentialStore
{
public:
    static QString read(const QString& service, const QString& account, QString* error = nullptr);
    static bool write(const QString& service, const QString& account, const QString& secret,
                      QString* error = nullptr);
    static bool remove(const QString& service, const QString& account, QString* error = nullptr);
};

#endif // CREDENTIALSTORE_H

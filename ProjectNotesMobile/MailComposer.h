// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <QString>
#include <QStringList>

class MailComposer
{
public:
    static bool isAvailable();
    static void present(const QStringList& toAddresses,
                        const QString&     subject,
                        const QString&     body);
};

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

#include <functional>

namespace PN::Comm {

// iCloud Mail, Calendar and Contacts share the Apple Account and the same
// app-specific password. The secret is fetched only when a handoff begins and
// is never persisted by a communication backend.
struct ICloudCredentials {
    QString account;
    QString password;
    // IMAP normally uses the iCloud Mail address, which can differ from the
    // Apple Account used by CalDAV/CardDAV. Empty means use `account`.
    QString mailUser;

    [[nodiscard]] bool valid() const
    {
        return !account.trimmed().isEmpty() && !password.isEmpty();
    }
};

using ICloudCredentialsProvider = std::function<ICloudCredentials()>;

} // namespace PN::Comm

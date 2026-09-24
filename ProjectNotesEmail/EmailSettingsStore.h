// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include <QSettings>

namespace PN::Comm {

// False for a backend this build does not provide: both Outlooks are Windows
// only and MacOS Email and Calendar is macOS only.
bool isBackendAvailableOnThisPlatform(BackendId backend);

class EmailSettingsStore {
public:
    explicit EmailSettingsStore(QSettings &settings);

    // A stored backend this platform lacks (a settings file carried over from
    // another machine) reads back as the platform default.
    BackendId preferredBackend() const;
    // Ignores a backend this platform lacks.
    void setPreferredBackend(BackendId backend);
    QString thunderbirdPath() const;
    void setThunderbirdPath(const QString &path);
    QString senderAddress() const;
    void setSenderAddress(const QString &address);

private:
    QSettings &m_settings;
};

} // namespace PN::Comm

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include <QSettings>

namespace PN::Comm {

class EmailSettingsStore {
public:
    explicit EmailSettingsStore(QSettings &settings);

    BackendId preferredBackend() const;
    void setPreferredBackend(BackendId backend);
    QString thunderbirdPath() const;
    void setThunderbirdPath(const QString &path);
    QString senderAddress() const;
    void setSenderAddress(const QString &address);

private:
    QSettings &m_settings;
};

} // namespace PN::Comm

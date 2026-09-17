// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include <QSettings>

namespace PN::Comm {

class EmailSettingsStore {
public:
    explicit EmailSettingsStore(QSettings &settings);

    static QString databaseKeyForPath(const QString &databasePath);

    BackendId preferredBackend() const;
    void setPreferredBackend(BackendId backend);
    QString thunderbirdPath() const;
    void setThunderbirdPath(const QString &path);
    QString exportSubfolder(Workflow workflow, const QString &databaseKey = {}) const;
    void setExportSubfolder(Workflow workflow, const QString &value, const QString &databaseKey = {});

private:
    QString reportKey(Workflow workflow, const QString &databaseKey) const;
    QSettings &m_settings;
};

} // namespace PN::Comm

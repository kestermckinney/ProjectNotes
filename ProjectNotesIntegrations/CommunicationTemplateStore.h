// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "TemplateTypes.h"

#include <QSettings>

namespace PN::Comm {

struct CommunicationTemplate {
    QString id;
    QString name;
    Workflow workflow = Workflow::SendMeetingNotes;
    QString subject;
    QString body;
    bool bundled = false;
    // Optional separate plain-text prose.  Older persisted templates simply
    // decode with an empty value and retain their existing rich body.
    QString plainBody;
};

class CommunicationTemplateStore final {
public:
    // Templates are stored in the open database's application_settings table.
    // `legacySettings` is read only to migrate the former per-machine QSettings
    // value the first time this database is used.
    CommunicationTemplateStore(QSettings &legacySettings, QString databaseKey,
                               QList<CommunicationTemplate> bundled = {});
    QList<CommunicationTemplate> templates(Workflow workflow) const;
    ValidationResult save(CommunicationTemplate value);
    // Removes a user override (or a user-created template).  A bundled template
    // with the same id becomes visible again on the next templates() call.
    ValidationResult reset(const QString &id);

private:
    QString storageKey() const;
    QString legacyStorageKey() const;
    QList<CommunicationTemplate> userTemplates(ValidationResult *validation = nullptr) const;
    bool writeUserTemplates(const QList<CommunicationTemplate> &values);
    QSettings &m_legacySettings;
    QString m_databaseKey;
    QList<CommunicationTemplate> m_bundled;
};

} // namespace PN::Comm

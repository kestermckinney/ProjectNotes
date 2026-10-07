// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "CommunicationTypes.h"

#include <QList>
#include <QStringList>

namespace PN::Comm {

// A user-defined meeting type.  Unlike an email workflow, which is one of a
// fixed set of native actions, meeting types are an open list: the user adds,
// renames and deletes them.  The id is generated once and never changes, so a
// rename keeps any later reference to the type intact.
struct MeetingTemplate {
    QString id;
    QString name;
    int durationMinutes = 60;
    QString subject;
    // Rich agenda prose placed in the invitation body.  The plain-text
    // DESCRIPTION a calendar file also needs is derived from it.
    QString agenda;
};

// Fields a meeting subject or agenda may insert.  A meeting is scheduled from
// a project, not from a single note or report, so only the project, client,
// preference and current-date fields apply.
QStringList meetingTemplateFieldPaths();

class MeetingTemplateStore final {
public:
    // Meeting types are stored as one JSON array in the open database's
    // application_settings table, alongside the email templates.
    QList<MeetingTemplate> templates(ValidationResult *validation = nullptr) const;
    // Called after opening a database. Seed an empty, valid store only;
    // a nonempty list and corrupt data are preserved. Deleting every type
    // leaves an empty list that will be seeded again at the next startup.
    ValidationResult initializeDefaults();
    ValidationResult save(MeetingTemplate value);
    ValidationResult remove(const QString &id);

    static QString storageKey();

private:
    bool write(const QList<MeetingTemplate> &values);
};

} // namespace PN::Comm

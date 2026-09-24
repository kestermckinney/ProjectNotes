// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesMeetings/MeetingTypes.h"

namespace PN::Comm {

struct StagedInvite {
    QString path;
    QString operationDirectory;
    ServiceError error;
    [[nodiscard]] bool ok() const { return error.code.isEmpty(); }
};

// Writes the meeting's invite.ics into its own ArtifactStore operation below
// `cacheDirectory`, so the file is removed with the rest of the staging tree.
StagedInvite stageMeetingInvite(const QString &cacheDirectory, const MeetingRequest &request,
                                const QDateTime &stamp = {});

} // namespace PN::Comm

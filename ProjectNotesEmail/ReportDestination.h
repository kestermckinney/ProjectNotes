// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

namespace PN::Comm {

// Resolves only a lexical, local publication target. Directory creation and
// the final hash-checked copy remain ArtifactStore responsibilities.
struct ReportDestination {
    // The lexical root is retained so materialization can verify every created
    // component remains inside the captured project folder.
    QString rootPath;
    QString directoryPath;
    QString filePath;
    ServiceError error;
    [[nodiscard]] bool ok() const { return error.code.isEmpty(); }
};

ReportDestination resolveReportDestination(const QString &projectFolderPath,
                                           const QString &exportSubfolder,
                                           const QString &fileName);

// Creates the requested relative directory below an existing local project
// folder. Existing or newly created path components must be real directories,
// never symlinks, and are rechecked canonically after creation.
ReportDestination ensureReportDestination(const ReportDestination &destination);

} // namespace PN::Comm

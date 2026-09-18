// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"

namespace PN::Comm {

class ThunderbirdEmailBackend final : public EmailBackend {
public:
    using ProcessLauncher = std::function<bool(const QString &, const QStringList &)>;
    using ExecutableProbe = std::function<bool(const QString &)>;
    ThunderbirdEmailBackend(QString executable, ProcessLauncher launcher = {});
    static std::optional<QStringList> composeArguments(const EmailRequest &request, const QString &bodyFile,
                                                       const QString &operationDirectory,
                                                       ValidationResult *validation = nullptr);
    // Discovery is deliberately explicit and injectable: the caller supplies
    // the platform candidate list and verifies executability without invoking
    // a shell or relying on PATH resolution.
    static QString resolveExecutable(const QString &configuredPath, const QStringList &candidates,
                                     ExecutableProbe probe = {});
    void handoff(EmailRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;
    // The body file and every request attachment must live below this exact
    // manifest-owned operation directory before the adapter exposes their URLs.
    void setBodyFile(const QUuid &operationId, QString path, QString operationDirectory);

private:
    QString m_program;
    QStringList m_commandArguments;
    ProcessLauncher m_launcher;
    struct StagedBodyFile { QString path; QString operationDirectory; };
    QHash<QUuid, StagedBodyFile> m_bodyFiles;
    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm

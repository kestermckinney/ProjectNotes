// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>
#include <QStringList>

#include <functional>

namespace PN::Comm {

// Starts the configured Thunderbird command without a shell.  Both the email
// compose adapter and the meeting (.ics import) adapter launch through here, so
// they resolve the saved command and grant Flatpak file access the same way.
class ThunderbirdLauncher final {
public:
    using ProcessLauncher = std::function<bool(const QString &, const QStringList &)>;

    explicit ThunderbirdLauncher(QString executable, ProcessLauncher launcher = {});

    [[nodiscard]] bool available() const;
    [[nodiscard]] QString program() const { return m_program; }
    // Appends `arguments` to the configured command.  For "flatpak run ...",
    // the sandbox is given read-only access to `readableDirectory` first.
    bool launch(const QStringList &arguments, const QString &readableDirectory) const;

private:
    QString m_program;
    QStringList m_commandArguments;
    ProcessLauncher m_launcher;
};

// True only for a readable regular file below the exact operation directory.
bool isOperationOwnedFile(const QString &path, const QString &operationDirectory);

} // namespace PN::Comm

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ThunderbirdLauncher.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace PN::Comm {
namespace {
bool isFlatpakLauncher(const QString &program, const QStringList &arguments)
{
    return QFileInfo(program).fileName() == QLatin1String("flatpak")
        && !arguments.isEmpty() && arguments.constFirst() == QLatin1String("run");
}
}

ThunderbirdLauncher::ThunderbirdLauncher(QString executable, ProcessLauncher launcher)
    : m_launcher(std::move(launcher))
{
    // A direct path (including one with spaces) remains a single program.  For
    // commands such as "flatpak run org.mozilla.Thunderbird", split only the
    // command syntax and pass every part directly to QProcess; never use a
    // shell to interpret the setting.
    const QString configured = executable.trimmed();
    if (QFileInfo(configured).isFile()) {
        m_program = configured;
    } else {
        const QStringList command = QProcess::splitCommand(configured);
        if (!command.isEmpty()) {
            m_program = QStandardPaths::findExecutable(command.constFirst());
            if (m_program.isEmpty())
                m_program = command.constFirst();
            m_commandArguments = command.sliced(1);
        }
    }
    if (!m_launcher) m_launcher = [](const QString &program, const QStringList &arguments) {
        return QProcess::startDetached(program, arguments);
    };
}

bool ThunderbirdLauncher::available() const
{
    const QFileInfo info(m_program);
    return info.isFile() && info.isExecutable();
}

bool ThunderbirdLauncher::launch(const QStringList &arguments, const QString &readableDirectory) const
{
    QStringList launchArguments = m_commandArguments;
    if (isFlatpakLauncher(m_program, launchArguments) && !readableDirectory.isEmpty())
        launchArguments.insert(1, QStringLiteral("--filesystem=%1:ro")
                                      .arg(QDir(readableDirectory).absolutePath()));
    launchArguments.append(arguments);
    return m_launcher(m_program, launchArguments);
}

bool isOperationOwnedFile(const QString &path, const QString &operationDirectory)
{
    const QFileInfo rootInfo(operationDirectory);
    const QFileInfo fileInfo(path);
    if (!rootInfo.isAbsolute() || !rootInfo.isDir() || !fileInfo.isAbsolute()
        || !fileInfo.isFile() || !fileInfo.isReadable())
        return false;
    const QString root = rootInfo.canonicalFilePath();
    const QString file = fileInfo.canonicalFilePath();
    if (root.isEmpty() || file.isEmpty())
        return false;
    const QString prefix = QDir::cleanPath(root) + QLatin1Char('/');
#ifdef Q_OS_WIN
    return QDir::cleanPath(file).startsWith(prefix, Qt::CaseInsensitive);
#else
    return QDir::cleanPath(file).startsWith(prefix, Qt::CaseSensitive);
#endif
}

} // namespace PN::Comm

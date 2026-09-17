// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ThunderbirdEmailBackend.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

namespace PN::Comm {
namespace {
QString quote(const QString &value) {
    QString result = value; result.replace('\\', "\\\\"); result.replace('\'', "\\'"); result.replace(',', "\\,");
    return QStringLiteral("'") + result + QStringLiteral("'");
}
bool invalid(const QString &value) { return value.contains('\r') || value.contains('\n'); }
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
}

ThunderbirdEmailBackend::ThunderbirdEmailBackend(QString executable, ProcessLauncher launcher)
    : m_executable(std::move(executable)), m_launcher(std::move(launcher))
{
    if (!m_launcher) m_launcher = [](const QString &program, const QStringList &arguments) {
        return QProcess::startDetached(program, arguments);
    };
}

QString ThunderbirdEmailBackend::resolveExecutable(const QString &configuredPath,
                                                    const QStringList &candidates,
                                                    ExecutableProbe probe)
{
    if (!probe)
        probe = [](const QString &path) { return QFileInfo(path).isFile() && QFileInfo(path).isExecutable(); };
    const QString configured = configuredPath.trimmed();
    if (!configured.isEmpty() && probe(configured))
        return configured;
    QSet<QString> examined;
    for (const QString &candidateValue : candidates) {
        const QString candidate = candidateValue.trimmed();
        if (!candidate.isEmpty() && !examined.contains(candidate)) {
            examined.insert(candidate);
            if (probe(candidate))
                return candidate;
        }
    }
    return {};
}

std::optional<QStringList> ThunderbirdEmailBackend::composeArguments(const EmailRequest &request, const QString &bodyFile,
                                                                       const QString &operationDirectory,
                                                                       ValidationResult *validation)
{
    auto fail = [validation](const QString &code, const QString &path) { if (validation) validation->addError(code, path); };
    QFileInfo body(bodyFile);
    if (!body.isFile() || !body.isReadable()) { fail("thunderbird-body-unavailable", "bodyFile"); return std::nullopt; }
    if (!isOperationOwnedFile(bodyFile, operationDirectory)) {
        fail("thunderbird-path-not-operation-owned", "bodyFile"); return std::nullopt;
    }
    if (invalid(request.subject)) { fail("email-header-injection", "subject"); return std::nullopt; }
    QStringList to, cc, bcc, attachments;
    for (const EmailAddress &recipient : request.recipients) {
        const QString address = recipient.address.trimmed();
        if (address.isEmpty()) continue;
        if (invalid(address) || address.contains(',') || address.contains(';') || !address.contains('@')) { fail("recipient-address-invalid", "recipients"); return std::nullopt; }
        if (recipient.role == RecipientRole::To) to.append(address); else if (recipient.role == RecipientRole::Cc) cc.append(address); else bcc.append(address);
    }
    if (to.isEmpty() && cc.isEmpty() && bcc.isEmpty() && !request.addressLaterExplicitlyChosen) { fail("recipient-required", "recipients"); return std::nullopt; }
    for (const Artifact &artifact : request.attachments) {
        QFileInfo info(artifact.absolutePath);
        if (!info.isFile() || !info.isReadable()) { fail("attachment-unavailable", artifact.displayName); return std::nullopt; }
        if (!isOperationOwnedFile(artifact.absolutePath, operationDirectory)) {
            fail("thunderbird-path-not-operation-owned", artifact.displayName); return std::nullopt;
        }
        attachments.append(QUrl::fromLocalFile(info.absoluteFilePath()).toString(QUrl::FullyEncoded));
    }
    QStringList fields;
    if (!to.isEmpty()) fields.append("to=" + quote(to.join(',')));
    if (!cc.isEmpty()) fields.append("cc=" + quote(cc.join(',')));
    if (!bcc.isEmpty()) fields.append("bcc=" + quote(bcc.join(',')));
    fields.append("subject=" + quote(request.subject));
    fields.append("body=" + quote(QUrl::fromLocalFile(body.absoluteFilePath()).toString(QUrl::FullyEncoded)));
    if (!attachments.isEmpty()) fields.append("attachment=" + quote(attachments.join(',')));
    return QStringList{"-compose", fields.join(',')};
}

void ThunderbirdEmailBackend::setBodyFile(const QUuid &operationId, QString path, QString operationDirectory)
{
    m_bodyFiles.insert(operationId, {std::move(path), std::move(operationDirectory)});
}
void ThunderbirdEmailBackend::cancel(const QUuid &operationId) { m_cancelled.insert(operationId); }

void ThunderbirdEmailBackend::handoff(EmailRequest request, Completion completion)
{
    EmailHandoffResult result; result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) { result.error = {"operation-cancelled", {}, RetryKind::None, OutcomeCertainty::Certain}; completion(result); return; }
    if (!QFileInfo(m_executable).isFile()) { result.error = {"thunderbird-unavailable", {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain}; completion(result); return; }
    const StagedBodyFile body = m_bodyFiles.take(request.operationId);
    ValidationResult validation;
    const auto arguments = composeArguments(request, body.path, body.operationDirectory, &validation);
    if (!arguments) { result.error = {validation.issues.constFirst().code, {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain}; completion(result); return; }
    if (!m_launcher(m_executable, *arguments)) { result.error = {"thunderbird-launch-failed", {}, RetryKind::RetryPresentation, OutcomeCertainty::Certain}; completion(result); return; }
    result.certainty = OutcomeCertainty::Uncertain; completion(result);
}

} // namespace PN::Comm

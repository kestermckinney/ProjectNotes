// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MailtoEmailBackend.h"

#include <QDesktopServices>

namespace PN::Comm {
namespace {
bool containsLineBreak(const QString &value) { return value.contains('\r') || value.contains('\n'); }
}

MailtoEmailBackend::MailtoEmailBackend(UrlOpener opener) : m_opener(std::move(opener))
{
    if (!m_opener) m_opener = [](const QUrl &url) { return QDesktopServices::openUrl(url); };
}

std::optional<QUrl> MailtoEmailBackend::buildUrl(const EmailRequest &request, ValidationResult *validation)
{
    auto error = [validation](const QString &code, const QString &field) {
        if (validation) validation->addError(code, field);
    };
    if (!request.attachments.isEmpty() || !request.html.trimmed().isEmpty()) {
        error("mailto-unsupported-content", "emailMode"); return std::nullopt;
    }
    if (containsLineBreak(request.subject)) {
        error("mailto-header-injection", "subject"); return std::nullopt;
    }
    QStringList to, cc, bcc;
    for (const EmailAddress &recipient : request.recipients) {
        const QString address = recipient.address.trimmed();
        if (address.isEmpty()) continue;
        if (containsLineBreak(address) || address.contains(',') || address.contains(';') || !address.contains('@')) {
            error("recipient-address-invalid", "recipients"); return std::nullopt;
        }
        if (recipient.role == RecipientRole::To) to.append(address);
        else if (recipient.role == RecipientRole::Cc) cc.append(address);
        else bcc.append(address);
    }
    if (to.isEmpty() && cc.isEmpty() && bcc.isEmpty() && !request.addressLaterExplicitlyChosen) {
        error("recipient-required", "recipients"); return std::nullopt;
    }
    QUrl url; url.setScheme(QStringLiteral("mailto")); url.setPath(to.join(','));
    QStringList query;
    const auto add = [&query](const QString &key, const QString &value) {
        query.append(key + QLatin1Char('=') + QString::fromLatin1(QUrl::toPercentEncoding(value)));
    };
    if (!cc.isEmpty()) add("cc", cc.join(','));
    if (!bcc.isEmpty()) add("bcc", bcc.join(','));
    if (!request.subject.isEmpty()) add("subject", request.subject);
    if (!request.plainText.isEmpty()) add("body", request.plainText);
    url.setQuery(query.join(QLatin1Char('&')), QUrl::StrictMode);
    if (url.toEncoded().size() > maximumEncodedLength) { error("mailto-url-too-long", "body"); return std::nullopt; }
    return url;
}

void MailtoEmailBackend::handoff(EmailRequest request, Completion completion)
{
    EmailHandoffResult result; result.operationId = request.operationId;
    if (m_cancelled.remove(request.operationId)) { result.error = {"operation-cancelled", {}, RetryKind::None, OutcomeCertainty::Certain}; completion(result); return; }
    if (m_launchAttempted.contains(request.operationId)) { result.error = {"mailto-launch-already-attempted", {}, RetryKind::None, OutcomeCertainty::Uncertain}; completion(result); return; }
    ValidationResult validation; const auto url = buildUrl(request, &validation);
    if (!url) { result.error = {validation.issues.constFirst().code, {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain}; completion(result); return; }
    m_launchAttempted.insert(request.operationId);
    if (!m_opener(*url)) { result.error = {"mailto-launch-failed", {}, RetryKind::RetryPresentation, OutcomeCertainty::Certain}; completion(result); return; }
    result.certainty = OutcomeCertainty::Uncertain; result.draftIdentity = url->toString(QUrl::FullyEncoded); completion(result);
}

void MailtoEmailBackend::cancel(const QUuid &operationId) { m_cancelled.insert(operationId); }

} // namespace PN::Comm

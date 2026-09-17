// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "GraphEmailBackend.h"

#include "ProjectNotesIntegrations/Office365Service.h"

#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <memory>

namespace PN::Comm {
namespace {
bool unsafeHeader(const QString &value) { return value.contains(u'\r') || value.contains(u'\n'); }
OutcomeCertainty draftCreateFailureCertainty(const HttpResponse &response)
{
    // A transport failure after the POST was dispatched gives no reliable answer
    // about whether Graph created a draft. Service preflight failures are local.
    if (response.statusCode == 0
        && response.error.code != QLatin1String("office365-not-authenticated")
        && response.error.code != QLatin1String("office365-transport-unavailable"))
        return OutcomeCertainty::Uncertain;
    return OutcomeCertainty::Certain;
}
bool attachmentMatchesArtifact(const Artifact &attachment, const QFileInfo &info)
{
    if (!info.isFile() || info.isSymLink() || !info.isReadable()
        || attachment.byteSize < 0 || attachment.sha256.isEmpty()
        || info.size() != attachment.byteSize)
        return false;
    QFile file(attachment.absolutePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
        hash.addData(file.read(65536));
    return hash.result() == attachment.sha256;
}
QJsonArray recipientsFor(const QList<EmailAddress> &recipients, RecipientRole role)
{
    QJsonArray result;
    for (const EmailAddress &recipient : recipients) {
        if (recipient.role != role || recipient.address.trimmed().isEmpty()) continue;
        result.append(QJsonObject{{QStringLiteral("emailAddress"),
                                   QJsonObject{{QStringLiteral("address"), recipient.address.trimmed()},
                                               {QStringLiteral("name"), recipient.displayName.trimmed()}}}});
    }
    return result;
}
}

GraphEmailBackend::GraphEmailBackend(Office365Service *service) : m_service(service) {}

bool GraphEmailBackend::isAllowedPresentationUrl(const QUrl &url)
{
    if (url.scheme() != QLatin1String("https") || !url.userInfo().isEmpty() || url.host().isEmpty())
        return false;
    const QString host = url.host().toLower();
    return host == QLatin1String("outlook.office.com")
        || host == QLatin1String("outlook.office365.com")
        || host == QLatin1String("outlook.office365.us");
}

void GraphEmailBackend::cancel(const QUuid &operationId)
{
    m_cancelled.insert(operationId);
}

void GraphEmailBackend::handoff(EmailRequest request, Completion completion)
{
    EmailHandoffResult result; result.operationId = request.operationId;
    auto fail = [&completion, result](QString code, OutcomeCertainty certainty = OutcomeCertainty::Certain,
                                      QString draftIdentity = {}) mutable {
        result.certainty = certainty;
        result.draftIdentity = std::move(draftIdentity);
        result.error = {std::move(code), {}, RetryKind::ReviewAndRetry, certainty};
        completion(std::move(result));
    };
    if (!m_service) { fail(QStringLiteral("graph-service-unavailable")); return; }
    if (m_cancelled.remove(request.operationId)) { fail(QStringLiteral("operation-cancelled")); return; }
    const Office365Account account = m_service->account();
    if (request.accountGeneration != account.generation) { fail(QStringLiteral("account-generation-changed")); return; }
    if (!account.grantedScopes.contains(QStringLiteral("Mail.ReadWrite"))) {
        fail(QStringLiteral("graph-mail-scope-required")); return;
    }
    if (unsafeHeader(request.subject)) { fail(QStringLiteral("email-header-injection")); return; }
    for (const EmailAddress &recipient : request.recipients)
        if (unsafeHeader(recipient.address) || recipient.address.contains(u',') || recipient.address.contains(u';') ||
            (!recipient.address.trimmed().isEmpty() && !recipient.address.contains(u'@'))) {
            fail(QStringLiteral("recipient-address-invalid")); return;
    }
    for (const Artifact &attachment : request.attachments) {
        const QFileInfo info(attachment.absolutePath);
        if (!attachmentMatchesArtifact(attachment, info)) { fail(QStringLiteral("attachment-integrity-mismatch")); return; }
    }

    QJsonObject draft{{QStringLiteral("subject"), request.subject},
                      {QStringLiteral("body"), QJsonObject{{QStringLiteral("contentType"), request.html.isEmpty() ? "Text" : "HTML"},
                                                             {QStringLiteral("content"), request.html.isEmpty() ? request.plainText : request.html}}},
                      {QStringLiteral("toRecipients"), recipientsFor(request.recipients, RecipientRole::To)},
                      {QStringLiteral("ccRecipients"), recipientsFor(request.recipients, RecipientRole::Cc)},
                      {QStringLiteral("bccRecipients"), recipientsFor(request.recipients, RecipientRole::Bcc)}};
    HttpRequest create; create.operationId = request.operationId; create.method = "POST";
    create.url = QUrl(QStringLiteral("https://graph.microsoft.com/v1.0/me/messages"));
    create.headers.insert("Content-Type", "application/json");
    create.body = QJsonDocument(draft).toJson(QJsonDocument::Compact);
    m_service->sendGraphRequest(std::move(create), [this, request = std::move(request), completion = std::move(completion)](HttpResponse response) mutable {
        EmailHandoffResult result; result.operationId = request.operationId;
        if (m_cancelled.remove(request.operationId)) { result.error = {"operation-cancelled", {}, RetryKind::None, OutcomeCertainty::Uncertain}; completion(result); return; }
        if (!response.error.code.isEmpty() || response.statusCode != 201) { result.error = {response.error.code.isEmpty() ? "graph-draft-create-failed" : response.error.code, {}, RetryKind::ReviewAndRetry, draftCreateFailureCertainty(response)}; completion(result); return; }
        const QJsonObject draftObject = QJsonDocument::fromJson(response.body).object();
        const QString draftId = draftObject.value("id").toString();
        if (draftId.isEmpty()) { result.error = {"graph-draft-id-missing", {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain}; completion(result); return; }
        const QUrl returnedPresentationUrl(draftObject.value("webLink").toString());
        const QUrl presentationUrl = isAllowedPresentationUrl(returnedPresentationUrl) ? returnedPresentationUrl : QUrl{};
        auto upload = std::make_shared<std::function<void(qsizetype)>>();
        *upload = [this, request, completion = std::move(completion), draftId, presentationUrl, upload](qsizetype index) mutable {
            EmailHandoffResult next; next.operationId = request.operationId; next.draftIdentity = draftId; next.presentationUrl = presentationUrl;
            if (m_cancelled.remove(request.operationId)) { next.error = {"operation-cancelled", {}, RetryKind::None, OutcomeCertainty::Uncertain}; completion(next); return; }
            if (index >= request.attachments.size()) { next.certainty = OutcomeCertainty::Certain; completion(next); return; }
            const Artifact &attachment = request.attachments.at(index);
            const QFileInfo info(attachment.absolutePath);
            if (info.size() > maximumSimpleAttachmentBytes) {
                QJsonObject session{{"AttachmentItem", QJsonObject{{"attachmentType", "file"}, {"name", attachment.displayName}, {"size", info.size()}}}};
                HttpRequest createSession; createSession.operationId = request.operationId; createSession.method = "POST";
                createSession.url = QUrl(QStringLiteral("https://graph.microsoft.com/v1.0/me/messages/") + QString::fromLatin1(QUrl::toPercentEncoding(draftId)) + QStringLiteral("/attachments/createUploadSession"));
                createSession.headers.insert("Content-Type", "application/json"); createSession.body = QJsonDocument(session).toJson(QJsonDocument::Compact);
                m_service->sendGraphRequest(std::move(createSession), [this, request, completion, draftId, presentationUrl, attachment, index, upload, info](HttpResponse response) mutable {
                    EmailHandoffResult failed; failed.operationId = request.operationId; failed.draftIdentity = draftId; failed.presentationUrl = presentationUrl;
                    if (m_cancelled.remove(request.operationId)) { failed.error = {"operation-cancelled", {}, RetryKind::None, OutcomeCertainty::Uncertain}; completion(failed); return; }
                    const QString uploadUrl = QJsonDocument::fromJson(response.body).object().value("uploadUrl").toString();
                    if (!response.error.code.isEmpty() || uploadUrl.isEmpty()) { failed.error = {response.error.code.isEmpty() ? "graph-upload-session-failed" : response.error.code, {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Uncertain}; completion(failed); return; }
                    auto putChunk = std::make_shared<std::function<void(qint64)>>();
                    *putChunk = [this, request, completion, draftId, presentationUrl, attachment, index, upload, putChunk, uploadUrl, info](qint64 offset) mutable {
                        QFile file(attachment.absolutePath);
                        EmailHandoffResult failed; failed.operationId = request.operationId; failed.draftIdentity = draftId; failed.presentationUrl = presentationUrl;
                        if (!file.open(QIODevice::ReadOnly) || !file.seek(offset)) { failed.error = {"attachment-unreadable", {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Uncertain}; completion(failed); return; }
                        const QByteArray bytes = file.read(qMin(uploadChunkBytes, info.size() - offset));
                        if (bytes.isEmpty()) { failed.error = {"attachment-read-failed", {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Uncertain}; completion(failed); return; }
                        HttpRequest put; put.operationId = request.operationId; put.method = "PUT"; put.url = QUrl(uploadUrl);
                        put.headers.insert("Content-Length", QByteArray::number(bytes.size()));
                        put.headers.insert("Content-Range", "bytes " + QByteArray::number(offset) + "-" + QByteArray::number(offset + bytes.size() - 1) + "/" + QByteArray::number(info.size()));
                        put.body = bytes;
                        m_service->sendUploadRequest(std::move(put), [this, request, putChunk, offset, bytes, info, upload, index, completion, failed](HttpResponse response) mutable {
                            if (m_cancelled.remove(request.operationId)) { auto result = failed; result.error = {"operation-cancelled", {}, RetryKind::None, OutcomeCertainty::Uncertain}; completion(result); return; }
                            if (!response.error.code.isEmpty() || (response.statusCode != 200 && response.statusCode != 201 && response.statusCode != 202)) { auto result = failed; result.error = {response.error.code.isEmpty() ? "graph-upload-chunk-failed" : response.error.code, {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Uncertain}; completion(result); return; }
                            const qint64 next = offset + bytes.size();
                            if (next >= info.size()) (*upload)(index + 1); else (*putChunk)(next);
                        });
                    };
                    (*putChunk)(0);
                });
                return;
            }
            QFile file(attachment.absolutePath);
            if (!file.open(QIODevice::ReadOnly)) { next.error = {"attachment-unreadable", {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Uncertain}; completion(next); return; }
            QJsonObject payload{{"@odata.type", "#microsoft.graph.fileAttachment"}, {"name", attachment.displayName},
                                {"contentType", attachment.mimeType}, {"contentBytes", QString::fromLatin1(file.readAll().toBase64())}};
            HttpRequest add; add.operationId = request.operationId; add.method = "POST";
            add.url = QUrl(QStringLiteral("https://graph.microsoft.com/v1.0/me/messages/") + QString::fromLatin1(QUrl::toPercentEncoding(draftId)) + QStringLiteral("/attachments"));
            add.headers.insert("Content-Type", "application/json"); add.body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
            m_service->sendGraphRequest(std::move(add), [this, request, upload, index, completion, next](HttpResponse response) mutable {
                if (m_cancelled.remove(request.operationId)) { auto cancelled = next; cancelled.error = {"operation-cancelled", {}, RetryKind::None, OutcomeCertainty::Uncertain}; completion(cancelled); return; }
                if (!response.error.code.isEmpty() || response.statusCode != 201) { auto failed = next; failed.error = {response.error.code.isEmpty() ? "graph-attachment-upload-failed" : response.error.code, {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Uncertain}; completion(failed); return; }
                (*upload)(index + 1);
            });
        };
        (*upload)(0);
    });
}
} // namespace PN::Comm

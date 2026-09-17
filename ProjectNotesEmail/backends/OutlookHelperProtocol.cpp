// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "OutlookHelperProtocol.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>

namespace PN::Comm {
namespace {
void reject(ValidationResult *validation, const QString &code)
{
    if (validation)
        validation->addError(code, QStringLiteral("outlookHelperProtocol"));
}

bool isOperationOwnedPath(const QString &path, const QString &operationDirectory)
{
    if (path.trimmed().isEmpty() || operationDirectory.trimmed().isEmpty())
        return false;
    const QFileInfo rootInfo(operationDirectory);
    const QFileInfo candidateInfo(path);
    if (!rootInfo.isAbsolute() || !candidateInfo.isAbsolute())
        return false;
    const QString root = QDir::cleanPath(rootInfo.absoluteFilePath());
    const QString candidate = QDir::cleanPath(candidateInfo.absoluteFilePath());
    const QString prefix = root.endsWith(QLatin1Char('/')) ? root : root + QLatin1Char('/');
#ifdef Q_OS_WIN
    return candidate.startsWith(prefix, Qt::CaseInsensitive);
#else
    return candidate.startsWith(prefix, Qt::CaseSensitive);
#endif
}
}

QString toStableString(OutlookHelperCommand command)
{
    switch (command) {
    case OutlookHelperCommand::Prepare: return QStringLiteral("prepare");
    case OutlookHelperCommand::Present: return QStringLiteral("present");
    case OutlookHelperCommand::Cancel: return QStringLiteral("cancel");
    case OutlookHelperCommand::Close: return QStringLiteral("close");
    }
    return {};
}

std::optional<OutlookHelperCommand> outlookHelperCommandFromStableString(const QString &command)
{
    if (command == QLatin1String("prepare")) return OutlookHelperCommand::Prepare;
    if (command == QLatin1String("present")) return OutlookHelperCommand::Present;
    if (command == QLatin1String("cancel")) return OutlookHelperCommand::Cancel;
    if (command == QLatin1String("close")) return OutlookHelperCommand::Close;
    return std::nullopt;
}

QString toStableString(OutlookHelperStage stage)
{
    switch (stage) {
    case OutlookHelperStage::Staged: return QStringLiteral("staged");
    case OutlookHelperStage::Presented: return QStringLiteral("presented");
    case OutlookHelperStage::Cancelled: return QStringLiteral("cancelled");
    case OutlookHelperStage::Failed: return QStringLiteral("failed");
    }
    return {};
}

std::optional<OutlookHelperStage> outlookHelperStageFromStableString(const QString &stage)
{
    if (stage == QLatin1String("staged")) return OutlookHelperStage::Staged;
    if (stage == QLatin1String("presented")) return OutlookHelperStage::Presented;
    if (stage == QLatin1String("cancelled")) return OutlookHelperStage::Cancelled;
    if (stage == QLatin1String("failed")) return OutlookHelperStage::Failed;
    return std::nullopt;
}

std::optional<QByteArray> encodeOutlookHelperRequest(const OutlookHelperRequest &request,
                                                     ValidationResult *validation)
{
    if (request.operationId.isNull()) { reject(validation, QStringLiteral("outlook-helper-operation-id-required")); return std::nullopt; }
    const QString command = toStableString(request.command);
    if (command.isEmpty()) { reject(validation, QStringLiteral("outlook-helper-request-invalid")); return std::nullopt; }
    const QJsonObject object{{QStringLiteral("protocolVersion"), 1},
                             {QStringLiteral("operationId"), request.operationId.toString(QUuid::WithoutBraces)},
                             {QStringLiteral("command"), command},
                             {QStringLiteral("payload"), request.payload}};
    QByteArray result = QJsonDocument(object).toJson(QJsonDocument::Compact);
    result.append('\n');
    if (result.size() > maximumOutlookHelperMessageBytes) { reject(validation, QStringLiteral("outlook-helper-message-too-large")); return std::nullopt; }
    return result;
}

std::optional<OutlookHelperRequest> decodeOutlookHelperRequest(const QByteArray &line,
                                                                ValidationResult *validation)
{
    if (line.isEmpty() || line.size() > maximumOutlookHelperMessageBytes || line.contains('\n') || line.contains('\r')) {
        reject(validation, QStringLiteral("outlook-helper-frame-invalid")); return std::nullopt;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        reject(validation, QStringLiteral("outlook-helper-json-invalid")); return std::nullopt;
    }
    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("protocolVersion")).toInt() != 1) {
        reject(validation, QStringLiteral("outlook-helper-protocol-version")); return std::nullopt;
    }
    const QUuid operationId(object.value(QStringLiteral("operationId")).toString());
    const auto command = outlookHelperCommandFromStableString(object.value(QStringLiteral("command")).toString());
    if (operationId.isNull() || !command || !object.value(QStringLiteral("payload")).isObject()) {
        reject(validation, QStringLiteral("outlook-helper-request-invalid")); return std::nullopt;
    }
    return OutlookHelperRequest{operationId, *command, object.value(QStringLiteral("payload")).toObject()};
}

std::optional<OutlookHelperRequest> makeOutlookPrepareRequest(const EmailRequest &request,
                                                               const QString &bodyFile,
                                                               const QString &operationDirectory,
                                                               ValidationResult *validation)
{
    auto invalid = [validation](const QString &code, const QString &field) {
        if (validation) validation->addError(code, field);
    };
    if (request.operationId.isNull()) { invalid(QStringLiteral("outlook-helper-operation-id-required"), QStringLiteral("operationId")); return std::nullopt; }
    if (request.subject.contains('\r') || request.subject.contains('\n')) { invalid(QStringLiteral("email-header-injection"), QStringLiteral("subject")); return std::nullopt; }
    if (bodyFile.trimmed().isEmpty()) { invalid(QStringLiteral("outlook-helper-body-required"), QStringLiteral("bodyFile")); return std::nullopt; }
    if (!isOperationOwnedPath(bodyFile, operationDirectory)) {
        invalid(QStringLiteral("outlook-helper-path-not-operation-owned"), QStringLiteral("bodyFile"));
        return std::nullopt;
    }
    QJsonArray recipients;
    for (const EmailAddress &recipient : request.recipients) {
        const QString address = recipient.address.trimmed();
        const QString role = toStableString(recipient.role);
        if (address.isEmpty() || address.contains('\r') || address.contains('\n') || address.contains(u',') || address.contains(u';')) {
            invalid(QStringLiteral("recipient-address-invalid"), QStringLiteral("recipients")); return std::nullopt;
        }
        if (role.isEmpty()) { invalid(QStringLiteral("recipient-role-invalid"), QStringLiteral("recipients")); return std::nullopt; }
        recipients.append(QJsonObject{{QStringLiteral("address"), address},
                                      {QStringLiteral("displayName"), recipient.displayName},
                                      {QStringLiteral("role"), role}});
    }
    QJsonArray attachments;
    for (const Artifact &artifact : request.attachments) {
        if (artifact.artifactId.isNull() || artifact.absolutePath.isEmpty()) {
            invalid(QStringLiteral("attachment-unavailable"), QStringLiteral("attachments")); return std::nullopt;
        }
        if (!isOperationOwnedPath(artifact.absolutePath, operationDirectory)) {
            invalid(QStringLiteral("outlook-helper-path-not-operation-owned"), QStringLiteral("attachments"));
            return std::nullopt;
        }
        attachments.append(QJsonObject{{QStringLiteral("artifactId"), artifact.artifactId.toString(QUuid::WithoutBraces)},
                                       {QStringLiteral("path"), artifact.absolutePath},
                                       {QStringLiteral("displayName"), artifact.displayName}});
    }
    return OutlookHelperRequest{request.operationId, OutlookHelperCommand::Prepare,
        {{QStringLiteral("subject"), request.subject}, {QStringLiteral("html"), request.html},
         {QStringLiteral("plainText"), request.plainText}, {QStringLiteral("bodyFile"), bodyFile},
         {QStringLiteral("recipients"), recipients}, {QStringLiteral("attachments"), attachments}}};
}

std::optional<QByteArray> encodeOutlookHelperResponse(const OutlookHelperResponse &response,
                                                      ValidationResult *validation)
{
    if (response.operationId.isNull()) { reject(validation, QStringLiteral("outlook-helper-operation-id-required")); return std::nullopt; }
    const QString stage = toStableString(response.stage);
    if (stage.isEmpty()) { reject(validation, QStringLiteral("outlook-helper-response-invalid")); return std::nullopt; }
    if (response.certainty < OutcomeCertainty::NotStarted || response.certainty > OutcomeCertainty::Uncertain) {
        reject(validation, QStringLiteral("outlook-helper-response-invalid")); return std::nullopt;
    }
    const QJsonObject object{{QStringLiteral("protocolVersion"), 1},
                             {QStringLiteral("operationId"), response.operationId.toString(QUuid::WithoutBraces)},
                             {QStringLiteral("stage"), stage},
                             {QStringLiteral("certainty"), static_cast<int>(response.certainty)},
                             {QStringLiteral("itemHandle"), response.itemHandle},
                             {QStringLiteral("error"), response.error.code}};
    QByteArray result = QJsonDocument(object).toJson(QJsonDocument::Compact);
    result.append('\n');
    if (result.size() > maximumOutlookHelperMessageBytes) { reject(validation, QStringLiteral("outlook-helper-message-too-large")); return std::nullopt; }
    return result;
}

std::optional<OutlookHelperResponse> decodeOutlookHelperResponse(const QByteArray &line,
                                                                  ValidationResult *validation)
{
    if (line.isEmpty() || line.size() > maximumOutlookHelperMessageBytes || line.contains('\n') || line.contains('\r')) {
        reject(validation, QStringLiteral("outlook-helper-frame-invalid")); return std::nullopt;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        reject(validation, QStringLiteral("outlook-helper-json-invalid")); return std::nullopt;
    }
    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("protocolVersion")).toInt() != 1) {
        reject(validation, QStringLiteral("outlook-helper-protocol-version")); return std::nullopt;
    }
    const QUuid operationId(object.value(QStringLiteral("operationId")).toString());
    const auto stage = outlookHelperStageFromStableString(object.value(QStringLiteral("stage")).toString());
    const int certainty = object.value(QStringLiteral("certainty")).toInt(-1);
    if (operationId.isNull() || !stage || certainty < static_cast<int>(OutcomeCertainty::NotStarted)
        || certainty > static_cast<int>(OutcomeCertainty::Uncertain)) {
        reject(validation, QStringLiteral("outlook-helper-response-invalid")); return std::nullopt;
    }
    return OutlookHelperResponse{operationId, *stage, static_cast<OutcomeCertainty>(certainty),
                                 object.value(QStringLiteral("itemHandle")).toString(),
                                 {object.value(QStringLiteral("error")).toString()}};
}

} // namespace PN::Comm

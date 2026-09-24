// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailTypes.h"

#include <QJsonObject>

#include <optional>

namespace PN::Comm {

// Transport-neutral NDJSON framing shared by the Windows parent adapter and
// its STA helper.  Keeping this pure makes protocol rejection testable on all
// hosts; neither message bodies nor credentials belong in process arguments.
enum class OutlookHelperCommand { Prepare, Present, Cancel, Close };
enum class OutlookHelperStage { Staged, Presented, Cancelled, Failed };

struct OutlookHelperRequest {
    QUuid operationId;
    OutlookHelperCommand command = OutlookHelperCommand::Prepare;
    QJsonObject payload;
};

struct OutlookHelperResponse {
    QUuid operationId;
    OutlookHelperStage stage = OutlookHelperStage::Failed;
    OutcomeCertainty certainty = OutcomeCertainty::NotStarted;
    QString itemHandle;
    ServiceError error;
};

constexpr qsizetype maximumOutlookHelperMessageBytes = 1024 * 1024;

QString toStableString(OutlookHelperCommand command);
std::optional<OutlookHelperCommand> outlookHelperCommandFromStableString(const QString &command);
QString toStableString(OutlookHelperStage stage);
std::optional<OutlookHelperStage> outlookHelperStageFromStableString(const QString &stage);
std::optional<QByteArray> encodeOutlookHelperRequest(const OutlookHelperRequest &request,
                                                     ValidationResult *validation = nullptr);
std::optional<OutlookHelperRequest> decodeOutlookHelperRequest(const QByteArray &line,
                                                                ValidationResult *validation = nullptr);
// Converts the immutable, already-staged email request into the helper's
// typed prepare payload. Every body/attachment path must be beneath the exact
// operation-owned directory. It deliberately carries no credentials and is
// never passed through a shell command line.
std::optional<OutlookHelperRequest> makeOutlookPrepareRequest(const EmailRequest &request,
                                                               const QString &bodyFile,
                                                               const QString &operationDirectory,
                                                               ValidationResult *validation = nullptr);
std::optional<QByteArray> encodeOutlookHelperResponse(const OutlookHelperResponse &response,
                                                      ValidationResult *validation = nullptr);
std::optional<OutlookHelperResponse> decodeOutlookHelperResponse(const QByteArray &line,
                                                                  ValidationResult *validation = nullptr);

} // namespace PN::Comm

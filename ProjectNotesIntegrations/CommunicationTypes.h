// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QMetaType>
#include <QString>
#include <QVariant>

#include <optional>

namespace PN::Comm {

enum class Workflow { SendMeetingNotes, TrackerItemsReport, StatusReport, MeetingNotesReport };
enum class EmailMode { InlineHtml, HtmlAttachment, PdfAttachment, None };
enum class BackendId { Graph, Mailto, OutlookClassic, Thunderbird, MacMail };
enum class RecipientRole { To, Cc, Bcc };
enum class IssueSeverity { Info, Warning, Error };
enum class RetryKind { None, RetryPresentation, RetryOperation, ReviewAndRetry };
enum class OutcomeCertainty { NotStarted, Certain, Uncertain };

struct ValidationIssue {
    QString code;
    IssueSeverity severity = IssueSeverity::Error;
    QString fieldPath;
    QString displayText;
    QString relatedId;
};

struct ValidationResult {
    QList<ValidationIssue> issues;

    [[nodiscard]] bool ok() const;
    void addError(QString code, QString fieldPath, QString displayText = {});
    void addWarning(QString code, QString fieldPath, QString displayText = {});
};

struct ServiceError {
    QString code;
    QString displayText;
    RetryKind retryKind = RetryKind::None;
    OutcomeCertainty certainty = OutcomeCertainty::NotStarted;
};

QString toStableString(Workflow value);
QString toStableString(EmailMode value);
QString toStableString(BackendId value);
QString toStableString(RecipientRole value);

std::optional<Workflow> workflowFromStableString(const QString &value);
std::optional<EmailMode> emailModeFromStableString(const QString &value);
std::optional<BackendId> backendIdFromStableString(const QString &value);
std::optional<RecipientRole> recipientRoleFromStableString(const QString &value);

void registerCommunicationMetaTypes();

} // namespace PN::Comm

Q_DECLARE_METATYPE(PN::Comm::Workflow)
Q_DECLARE_METATYPE(PN::Comm::EmailMode)
Q_DECLARE_METATYPE(PN::Comm::BackendId)
Q_DECLARE_METATYPE(PN::Comm::RecipientRole)
Q_DECLARE_METATYPE(PN::Comm::ValidationIssue)
Q_DECLARE_METATYPE(PN::Comm::ValidationResult)
Q_DECLARE_METATYPE(PN::Comm::ServiceError)

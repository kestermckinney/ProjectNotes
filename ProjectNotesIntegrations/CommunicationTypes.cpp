// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "CommunicationTypes.h"

#include <initializer_list>
#include <utility>

namespace PN::Comm {

bool ValidationResult::ok() const
{
    for (const auto &issue : issues) {
        if (issue.severity == IssueSeverity::Error)
            return false;
    }
    return true;
}

void ValidationResult::addError(QString code, QString fieldPath, QString displayText)
{
    issues.append({std::move(code), IssueSeverity::Error, std::move(fieldPath),
                   std::move(displayText), {}});
}

void ValidationResult::addWarning(QString code, QString fieldPath, QString displayText)
{
    issues.append({std::move(code), IssueSeverity::Warning, std::move(fieldPath),
                   std::move(displayText), {}});
}

namespace {
template <typename Enum>
std::optional<Enum> enumFromString(const QString &value,
                                   std::initializer_list<std::pair<const char *, Enum>> values)
{
    for (const auto &[stableName, enumValue] : values) {
        if (value == QLatin1String(stableName))
            return enumValue;
    }
    return std::nullopt;
}
} // namespace

QString toStableString(Workflow value)
{
    switch (value) {
    case Workflow::SendMeetingNotes: return QStringLiteral("send-meeting-notes");
    case Workflow::TrackerItemsReport: return QStringLiteral("tracker-items-report");
    case Workflow::StatusReport: return QStringLiteral("status-report");
    case Workflow::MeetingNotesReport: return QStringLiteral("meeting-notes-report");
    }
    return {};
}

QString toStableString(EmailMode value)
{
    switch (value) {
    case EmailMode::InlineHtml: return QStringLiteral("inline-html");
    case EmailMode::HtmlAttachment: return QStringLiteral("html-attachment");
    case EmailMode::PdfAttachment: return QStringLiteral("pdf-attachment");
    case EmailMode::None: return QStringLiteral("none");
    }
    return {};
}

QString toStableString(BackendId value)
{
    switch (value) {
    case BackendId::Graph: return QStringLiteral("graph");
    case BackendId::Mailto: return QStringLiteral("mailto");
    case BackendId::OutlookClassic: return QStringLiteral("outlook-classic");
    case BackendId::Thunderbird: return QStringLiteral("thunderbird");
    case BackendId::MacMail: return QStringLiteral("mac-mail");
    }
    return {};
}

QString toStableString(RecipientRole value)
{
    switch (value) {
    case RecipientRole::To: return QStringLiteral("to");
    case RecipientRole::Cc: return QStringLiteral("cc");
    case RecipientRole::Bcc: return QStringLiteral("bcc");
    }
    return {};
}

std::optional<Workflow> workflowFromStableString(const QString &value)
{
    return enumFromString<Workflow>(value, {{"send-meeting-notes", Workflow::SendMeetingNotes},
                                             {"tracker-items-report", Workflow::TrackerItemsReport},
                                             {"status-report", Workflow::StatusReport},
                                             {"meeting-notes-report", Workflow::MeetingNotesReport}});
}

std::optional<EmailMode> emailModeFromStableString(const QString &value)
{
    return enumFromString<EmailMode>(value, {{"inline-html", EmailMode::InlineHtml},
                                              {"html-attachment", EmailMode::HtmlAttachment},
                                              {"pdf-attachment", EmailMode::PdfAttachment},
                                              {"none", EmailMode::None}});
}

std::optional<BackendId> backendIdFromStableString(const QString &value)
{
    return enumFromString<BackendId>(value, {{"graph", BackendId::Graph}, {"mailto", BackendId::Mailto},
                                               {"outlook-classic", BackendId::OutlookClassic},
                                               {"thunderbird", BackendId::Thunderbird}, {"mac-mail", BackendId::MacMail}});
}

std::optional<RecipientRole> recipientRoleFromStableString(const QString &value)
{
    return enumFromString<RecipientRole>(value, {{"to", RecipientRole::To}, {"cc", RecipientRole::Cc},
                                                   {"bcc", RecipientRole::Bcc}});
}

void registerCommunicationMetaTypes()
{
    qRegisterMetaType<Workflow>();
    qRegisterMetaType<EmailMode>();
    qRegisterMetaType<BackendId>();
    qRegisterMetaType<RecipientRole>();
    qRegisterMetaType<ValidationIssue>();
    qRegisterMetaType<ValidationResult>();
    qRegisterMetaType<ServiceError>();
}

} // namespace PN::Comm

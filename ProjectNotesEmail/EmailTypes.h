// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesIntegrations/CommunicationTypes.h"

#include <QByteArray>
#include <QDate>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <optional>

namespace PN::Comm {

struct SourceContext {
    QString databaseKey;
    quint64 databaseGeneration = 0;
    QString projectId;
    QStringList noteIds;
    Workflow workflow = Workflow::SendMeetingNotes;
};

struct TrackerFilters {
    QSet<QString> itemTypes {QStringLiteral("Tracker")};
    QSet<QString> statuses {QStringLiteral("New"), QStringLiteral("Assigned")};
};

struct ReportOptions {
    Workflow workflow = Workflow::TrackerItemsReport;
    QDate reportingDate;
    bool internalReport = false;
    bool displayPdf = false;
    bool retainHtml = false;
    EmailMode emailMode = EmailMode::InlineHtml;
    std::optional<TrackerFilters> tracker;
};

struct EmailAddress {
    QString displayName;
    QString address;
    RecipientRole role = RecipientRole::To;
};

struct Artifact {
    QUuid artifactId;
    QString absolutePath;
    QString displayName;
    QString mimeType;
    qint64 byteSize = 0;
    QByteArray sha256;
    bool generatedByApp = false;
};

struct EmailRequest {
    QUuid operationId;
    SourceContext source;
    quint64 previewRevision = 0;
    BackendId backend = BackendId::Mailto;
    QString accountKey;
    quint64 accountGeneration = 0;
    QList<EmailAddress> recipients;
    QString subject;
    QString plainText;
    QString html;
    QList<Artifact> attachments;
    bool addressLaterExplicitlyChosen = false;
};

ReportOptions defaultReportOptions(Workflow workflow, const QDate &today);
ValidationResult validate(const SourceContext &context);
ValidationResult validate(const ReportOptions &options);

} // namespace PN::Comm

Q_DECLARE_METATYPE(PN::Comm::SourceContext)
Q_DECLARE_METATYPE(PN::Comm::TrackerFilters)
Q_DECLARE_METATYPE(PN::Comm::ReportOptions)
Q_DECLARE_METATYPE(PN::Comm::EmailAddress)
Q_DECLARE_METATYPE(PN::Comm::Artifact)
Q_DECLARE_METATYPE(PN::Comm::EmailRequest)

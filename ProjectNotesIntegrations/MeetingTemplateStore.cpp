// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MeetingTemplateStore.h"
#include "TemplateParser.h"
#include "databaseobjects.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace PN::Comm {
namespace {

QJsonObject encode(const MeetingTemplate &value)
{
    return {{"id", value.id}, {"name", value.name}, {"durationMinutes", value.durationMinutes}, {"subject", value.subject},
            {"agenda", value.agenda}, {"plainAgenda", value.plainAgenda}};
}

std::optional<MeetingTemplate> decode(const QJsonObject &object)
{
    MeetingTemplate value;
    value.id = object.value("id").toString();
    if (value.id.isEmpty()) return std::nullopt;
    value.name = object.value("name").toString();
    const int duration = object.value("durationMinutes").toInt();
    if (duration > 0) value.durationMinutes = duration;
    value.subject = object.value("subject").toString();
    value.agenda = object.value("agenda").toString();
    value.plainAgenda = object.value("plainAgenda").toString();
    return value;
}

} // namespace

QStringList meetingTemplateFieldPaths()
{
    return {QStringLiteral("project.number"), QStringLiteral("project.name"), QStringLiteral("client.name"),
            QStringLiteral("preferences.managerName"), QStringLiteral("preferences.managingCompanyName"),
            QStringLiteral("current.date")};
}

QString MeetingTemplateStore::storageKey()
{
    return QStringLiteral("Meeting/v1/Templates");
}

QList<MeetingTemplate> MeetingTemplateStore::templates(ValidationResult *validation) const
{
    if (!global_DBObjects.isOpen()) {
        if (validation) validation->addError("template-store-unavailable", storageKey());
        return {};
    }
    const QByteArray stored = global_DBObjects.loadParameter(storageKey()).toUtf8();
    const QJsonDocument document = QJsonDocument::fromJson(stored);
    // As with email templates, an empty value is a valid first run but a
    // malformed one must not be silently treated as "no meeting types".
    if ((!stored.trimmed().isEmpty() && document.isNull()) || (!document.isNull() && !document.isArray())) {
        if (validation) validation->addError("template-store-corrupt", storageKey());
        return {};
    }
    QList<MeetingTemplate> result;
    for (const QJsonValue &entry : document.array()) {
        const auto value = decode(entry.toObject());
        if (!value) { if (validation) validation->addError("template-store-corrupt", storageKey()); continue; }
        result.append(*value);
    }
    return result;
}

bool MeetingTemplateStore::write(const QList<MeetingTemplate> &values)
{
    QJsonArray array; for (const MeetingTemplate &value : values) array.append(encode(value));
    return global_DBObjects.saveParameter(storageKey(),
                                          QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
}

ValidationResult MeetingTemplateStore::save(MeetingTemplate value)
{
    ValidationResult result;
    if (value.id.trimmed().isEmpty() || value.name.trimmed().isEmpty()) {
        result.addError("template-invalid", "template"); return result;
    }
    // Incomplete drafts may be stored for later repair, but a CR/LF in the
    // subject is unsafe to persist as a message header.
    for (const ValidationIssue &issue : validateTemplateSubject(value.subject).issues)
        if (issue.code != QStringLiteral("template-subject-required"))
            result.issues.append(issue);
    if (!result.ok()) return result;
    if (value.durationMinutes <= 0) value.durationMinutes = 60;
    QList<MeetingTemplate> values = templates(&result);
    if (!result.ok()) return result;
    const auto existing = std::find_if(values.begin(), values.end(),
                                       [&value](const MeetingTemplate &entry) { return entry.id == value.id; });
    if (existing != values.end()) *existing = value;
    else values.append(value);
    if (!write(values)) result.addError("template-store-write-failed", storageKey());
    return result;
}

ValidationResult MeetingTemplateStore::remove(const QString &id)
{
    ValidationResult result;
    if (id.trimmed().isEmpty()) {
        result.addError("template-invalid", "template");
        return result;
    }
    QList<MeetingTemplate> values = templates(&result);
    if (!result.ok()) return result;
    const auto end = std::remove_if(values.begin(), values.end(),
                                    [&id](const MeetingTemplate &value) { return value.id == id; });
    if (end != values.end()) {
        values.erase(end, values.end());
        if (!write(values)) result.addError("template-store-write-failed", storageKey());
    }
    return result;
}

} // namespace PN::Comm

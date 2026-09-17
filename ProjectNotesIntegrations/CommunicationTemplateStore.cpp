// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "CommunicationTemplateStore.h"
#include "TemplateParser.h"
#include "databaseobjects.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace PN::Comm {
namespace {

QJsonObject encode(const CommunicationTemplate &value)
{
    return {{"id", value.id}, {"name", value.name}, {"workflow", toStableString(value.workflow)},
            {"subject", value.subject}, {"body", value.body},
            {"plainBody", value.plainBody}};
}

std::optional<CommunicationTemplate> decode(const QJsonObject &object)
{
    const auto workflow = workflowFromStableString(object.value("workflow").toString());
    const QString id = object.value("id").toString();
    if (!workflow || id.isEmpty()) return std::nullopt;
    return CommunicationTemplate{id, object.value("name").toString(), *workflow,
                                 object.value("subject").toString(), object.value("body").toString(), false,
                                 object.value("plainBody").toString()};
}

} // namespace

CommunicationTemplateStore::CommunicationTemplateStore(QSettings &settings, QString databaseKey,
                                                       QList<CommunicationTemplate> bundled)
    : m_legacySettings(settings), m_databaseKey(std::move(databaseKey)), m_bundled(std::move(bundled))
{
    m_legacySettings.setFallbacksEnabled(false);
    for (CommunicationTemplate &value : m_bundled) value.bundled = true;
}

QString CommunicationTemplateStore::storageKey() const
{
    return QStringLiteral("Email/v1/Templates");
}

QString CommunicationTemplateStore::legacyStorageKey() const
{
    return storageKey() + QLatin1Char('/')
        + (m_databaseKey.isEmpty() ? QStringLiteral("global") : m_databaseKey);
}

QList<CommunicationTemplate> CommunicationTemplateStore::userTemplates(ValidationResult *validation) const
{
    if (!global_DBObjects.isOpen()) {
        if (validation) validation->addError("template-store-unavailable", storageKey());
        return {};
    }

    QByteArray stored = global_DBObjects.loadParameter(storageKey()).toUtf8();
    // Versions before database-backed email templates stored a separate value
    // for each database path in QSettings.  Bring that value forward once, but
    // deliberately retain it locally as a recovery copy.
    if (stored.isEmpty()) {
        const QByteArray legacy = m_legacySettings.value(legacyStorageKey()).toByteArray();
        if (!legacy.isEmpty()) {
            if (!global_DBObjects.saveParameter(storageKey(), QString::fromUtf8(legacy))) {
                if (validation) validation->addError("template-store-write-failed", storageKey());
                return {};
            }
            stored = legacy;
        }
    }
    const QJsonDocument document = QJsonDocument::fromJson(stored);
    // QJsonDocument::isNull() represents both an empty value and a parse
    // failure. Only the former is a valid first-run store; malformed persisted
    // data must not be silently treated as no user templates.
    if ((!stored.trimmed().isEmpty() && document.isNull()) || (!document.isNull() && !document.isArray())) {
        if (validation) validation->addError("template-store-corrupt", storageKey());
        return {};
    }
    QList<CommunicationTemplate> result;
    for (const QJsonValue &entry : document.array()) {
        const auto value = decode(entry.toObject());
        if (!value) { if (validation) validation->addError("template-store-corrupt", storageKey()); continue; }
        // Definitions previously imported from the Python plug-in are marked
        // with their original source. They are intentionally not available to
        // native email workflows.
        if (!entry.toObject().value(QStringLiteral("legacyOriginal")).toString().isEmpty()
            || value->id.startsWith(QStringLiteral("legacy-")))
            continue;
        result.append(*value);
    }
    return result;
}

QList<CommunicationTemplate> CommunicationTemplateStore::templates(Workflow workflow) const
{
    QList<CommunicationTemplate> result;
    for (const CommunicationTemplate &value : m_bundled) if (value.workflow == workflow) result.append(value);
    for (const CommunicationTemplate &value : userTemplates()) {
        if (value.workflow != workflow) continue;
        for (int i = result.size() - 1; i >= 0; --i) if (result.at(i).id == value.id) result.removeAt(i);
        result.append(value);
    }
    return result;
}

bool CommunicationTemplateStore::writeUserTemplates(const QList<CommunicationTemplate> &values)
{
    QJsonArray array; for (const CommunicationTemplate &value : values) array.append(encode(value));
    return global_DBObjects.saveParameter(storageKey(),
                                          QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
}

ValidationResult CommunicationTemplateStore::save(CommunicationTemplate value)
{
    ValidationResult result;
    if (value.id.trimmed().isEmpty() || value.name.trimmed().isEmpty()) {
        result.addError("template-invalid", "template"); return result;
    }
    // Editing storage must retain incomplete drafts for later repair.  Subject
    // presence and unresolved fields are enforced by preview/application, but
    // a CR/LF header injection is unsafe even to persist as an email subject.
    for (const ValidationIssue &issue : validateTemplateSubject(value.subject).issues)
        if (issue.code != QStringLiteral("template-subject-required"))
            result.issues.append(issue);
    if (!result.ok()) return result;
    value.bundled = false;
    QList<CommunicationTemplate> values = userTemplates(&result);
    if (!result.ok()) return result;
    for (CommunicationTemplate &existing : values) {
        if (existing.id != value.id) continue;
        existing = value;
        if (!writeUserTemplates(values)) result.addError("template-store-write-failed", storageKey());
        return result;
    }
    values.append(value);
    if (!writeUserTemplates(values)) result.addError("template-store-write-failed", storageKey());
    return result;
}

ValidationResult CommunicationTemplateStore::reset(const QString &id)
{
    ValidationResult result;
    if (id.trimmed().isEmpty()) {
        result.addError("template-invalid", "template");
        return result;
    }
    QList<CommunicationTemplate> values = userTemplates(&result);
    if (!result.ok()) return result;
    const auto end = std::remove_if(values.begin(), values.end(), [&id](const CommunicationTemplate &value) {
        return value.id == id;
    });
    if (end != values.end()) {
        values.erase(end, values.end());
        if (!writeUserTemplates(values)) result.addError("template-store-write-failed", storageKey());
    }
    return result;
}

} // namespace PN::Comm

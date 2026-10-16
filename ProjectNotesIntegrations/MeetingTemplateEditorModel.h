// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "MeetingTemplateStore.h"

#include <QObject>
#include <QVariantList>

#include <optional>

namespace PN::Comm {

// QML adapter over MeetingTemplateStore, the meeting counterpart of
// TemplateEditorModel.  Where the email editor switches between fixed
// workflows, this one selects among the user's own meeting types, so it also
// creates, duplicates and deletes them.  Fields are inserted as literal tokens;
// TemplateParser remains the single parser/validator.
class MeetingTemplateEditorModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList templates READ templates NOTIFY changed)
    Q_PROPERTY(QStringList availableFields READ availableFields CONSTANT)
    Q_PROPERTY(QString selectedTemplateId READ selectedTemplateId NOTIFY changed)
    Q_PROPERTY(bool hasDraft READ hasDraft NOTIFY changed)
    Q_PROPERTY(QString draftName READ draftName WRITE setDraftName NOTIFY changed)
    Q_PROPERTY(int draftDurationMinutes READ draftDurationMinutes WRITE setDraftDurationMinutes NOTIFY changed)
    Q_PROPERTY(QString draftSubject READ draftSubject WRITE setDraftSubject NOTIFY changed)
    Q_PROPERTY(QString draftRichAgenda READ draftRichAgenda WRITE setDraftRichAgenda NOTIFY changed)
    Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY changed)
public:
    explicit MeetingTemplateEditorModel(QObject *parent = nullptr);

    QVariantList templates() const;
    QStringList availableFields() const { return meetingTemplateFieldPaths(); }
    QString selectedTemplateId() const { return m_selectedId; }
    // True while the editor holds a meeting type, saved or newly begun.
    bool hasDraft() const { return !m_selectedId.isEmpty(); }
    QString draftName() const { return m_draft.name; }
    int draftDurationMinutes() const { return m_draft.durationMinutes; }
    QString draftSubject() const { return m_draft.subject; }
    QString draftRichAgenda() const { return m_draft.agenda; }
    QString diagnostic() const { return m_diagnostic; }

    void setDraftName(const QString &value);
    void setDraftDurationMinutes(int value);
    void setDraftSubject(const QString &value);
    void setDraftRichAgenda(const QString &value);

    // Switching database discards the draft rather than saving it into the
    // new database's settings.
    void setDatabaseKey(QString databaseKey);

    Q_INVOKABLE bool selectTemplate(const QString &id);
    // Starts an unsaved meeting type.  A new draft left completely blank is
    // discarded by save() instead of being stored without a name.
    Q_INVOKABLE void beginNew();
    Q_INVOKABLE bool duplicateSelected();
    Q_INVOKABLE bool deleteSelected();
    Q_INVOKABLE bool resetDefaults();
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool appendField(const QString &path, const QString &target);
    Q_INVOKABLE bool validateDraft();

    // Scheduling uses only persisted meeting types; an unsaved settings draft
    // never changes an invitation.
    [[nodiscard]] QList<MeetingTemplate> savedTemplates() const { return m_templates; }
    [[nodiscard]] std::optional<MeetingTemplate> templateById(const QString &id) const;

signals:
    void changed();

private:
    void reload(const QString &preferredId = {});
    void setDiagnostic(const ValidationResult &result);
    ValidationResult validate() const;
    bool draftIsBlank() const;

    QString m_databaseKey;
    QList<MeetingTemplate> m_templates;
    MeetingTemplate m_draft;
    // The draft was begun in the editor and has not been stored yet.
    bool m_draftIsNew = false;
    QString m_selectedId;
    QString m_diagnostic;
};

} // namespace PN::Comm

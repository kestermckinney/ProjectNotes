// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "CommunicationTemplateStore.h"

#include <QObject>
#include <QSettings>
#include <QVariantList>

#include <memory>
#include <optional>

namespace PN::Comm {

// A deliberately small QML adapter around the persistent template store.  It
// contains no JavaScript/template evaluation: fields are inserted as literal
// tokens and TemplateParser remains the single parser/validator.
class TemplateEditorModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString workflow READ workflow WRITE setWorkflow NOTIFY changed)
    Q_PROPERTY(QVariantList templates READ templates NOTIFY changed)
    Q_PROPERTY(QStringList availableFields READ availableFields NOTIFY changed)
    Q_PROPERTY(QStringList availableBodyFields READ availableBodyFields NOTIFY changed)
    Q_PROPERTY(QString selectedTemplateId READ selectedTemplateId NOTIFY changed)
    Q_PROPERTY(QString draftName READ draftName WRITE setDraftName NOTIFY changed)
    Q_PROPERTY(QString draftSubject READ draftSubject WRITE setDraftSubject NOTIFY changed)
    Q_PROPERTY(QString draftRichBody READ draftRichBody WRITE setDraftRichBody NOTIFY changed)
    Q_PROPERTY(QString draftPlainBody READ draftPlainBody WRITE setDraftPlainBody NOTIFY changed)
    Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY changed)
public:
    explicit TemplateEditorModel(QString organization, QString databaseKey = {}, QObject *parent = nullptr);

    QString workflow() const;
    void setWorkflow(const QString &workflow);
    QVariantList templates() const;
    QStringList availableFields() const;
    QStringList availableBodyFields() const;
    QString selectedTemplateId() const { return m_selectedId; }
    QString draftName() const { return m_draft.name; }
    QString draftSubject() const { return m_draft.subject; }
    QString draftRichBody() const { return m_draft.body; }
    QString draftPlainBody() const { return m_draftPlainBody; }
    QString diagnostic() const { return m_diagnostic; }

    void setDraftName(const QString &value);
    void setDraftSubject(const QString &value);
    void setDraftRichBody(const QString &value);
    void setDraftPlainBody(const QString &value);

    // Switching database must discard the prior draft rather than accidentally
    // saving it into the new database's settings scope.
    void setDatabaseKey(QString databaseKey);

    Q_INVOKABLE bool selectTemplate(const QString &id);
    Q_INVOKABLE void beginNew();
    Q_INVOKABLE bool duplicateSelected();
    Q_INVOKABLE bool resetSelected();
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool appendField(const QString &path, const QString &target);
    Q_INVOKABLE bool validateDraft();

    // Composition roots use only persisted templates returned by this lookup;
    // an unsaved settings draft can never change an email review.
    [[nodiscard]] std::optional<CommunicationTemplate> templateById(const QString &id) const;

signals:
    void changed();

private:
    Workflow currentWorkflow() const;
    void reload(const QString &preferredId = {});
    void setDiagnostic(const ValidationResult &result);
    ValidationResult validate() const;

    std::unique_ptr<QSettings> m_settings;
    QString m_databaseKey;
    Workflow m_workflow = Workflow::SendMeetingNotes;
    QList<CommunicationTemplate> m_templates;
    CommunicationTemplate m_draft;
    QString m_draftPlainBody;
    QString m_selectedId;
    QString m_diagnostic;
};

} // namespace PN::Comm

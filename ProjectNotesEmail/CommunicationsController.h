// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailContentBuilder.h"
#include "EmailService.h"

#include <QObject>

namespace PN::Comm {

enum class PreparationStage { Editing, Reviewing, RevalidatingSource, PreparingBackend, Completed, Failed };

// State holder for a single immutable handoff. It intentionally coordinates
// only preparation/dispatch; snapshots, rendering, and artifacts remain in
// their dedicated services.
class CommunicationsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(quint64 previewRevision READ previewRevision NOTIFY stateChanged)
    Q_PROPERTY(QString stageName READ stageName NOTIFY stateChanged)
    Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY stateChanged)
    Q_PROPERTY(QString subject READ subject NOTIFY stateChanged)
    Q_PROPERTY(QString plainText READ plainText NOTIFY stateChanged)
    Q_PROPERTY(QString htmlBody READ htmlBody NOTIFY stateChanged)
    Q_PROPERTY(bool inlineHtmlMode READ inlineHtmlMode NOTIFY stateChanged)
    Q_PROPERTY(QStringList attachmentNames READ attachmentNames NOTIFY stateChanged)
    Q_PROPERTY(QStringList generatedAttachmentNames READ generatedAttachmentNames NOTIFY stateChanged)
    Q_PROPERTY(QString draftIdentity READ draftIdentity NOTIFY stateChanged)
    Q_PROPERTY(QUrl presentationUrl READ presentationUrl NOTIFY stateChanged)
public:
    explicit CommunicationsController(EmailService *emailService, QObject *parent = nullptr);
    // Clears the displayed review before an asynchronous snapshot/build begins.
    // This prevents a new action from momentarily presenting a prior action's
    // subject/body as if it belonged to the new persisted selection.
    bool beginReviewPreparation();
    bool setPreparation(EmailPreparation preparation);
    // Used by the composition root when a review-owned model (for example,
    // recipient overrides) rejects the final handoff before a backend call.
    void setReviewValidation(ValidationResult validation);
    // The composition root freezes review-local edits while it reloads the
    // source snapshot. `handoffAfterSourceRevalidation` is the only path that
    // may dispatch while this guard is active.
    bool beginSourceRevalidation();
    bool handoffAfterSourceRevalidation();
    bool handoffAfterSourceRevalidation(QList<EmailAddress> recipients, bool addressLater);
    void failSourceRevalidation(ValidationResult validation);
    Q_INVOKABLE bool handoff();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE bool setSubject(QString subject);
    // Editable HTML is restricted to inline mode. Attachment modes retain the
    // manifest-owned generated file rather than presenting a divergent body.
    Q_INVOKABLE bool setHtmlBody(QString html);
    // Composition-root only: the ArtifactStore has already made an immutable
    // operation-owned copy before this is appended to the review.
    bool appendAttachment(Artifact attachment);

    [[nodiscard]] bool busy() const { return m_busy; }
    [[nodiscard]] quint64 previewRevision() const { return m_revision; }
    [[nodiscard]] PreparationStage stage() const { return m_stage; }
    [[nodiscard]] QString stageName() const;
    [[nodiscard]] QString diagnostic() const;
    [[nodiscard]] QString subject() const { return m_preparation.document.defaultSubject; }
    [[nodiscard]] QString plainText() const { return m_preparation.document.plainText; }
    [[nodiscard]] QString htmlBody() const { return m_preparation.document.emailFragment; }
    [[nodiscard]] bool inlineHtmlMode() const { return m_preparation.mode == EmailMode::InlineHtml; }
    [[nodiscard]] QStringList attachmentNames() const {
        QStringList names;
        for (const Artifact &attachment : m_preparation.attachments)
            names.append(attachment.displayName);
        return names;
    }
    [[nodiscard]] QStringList generatedAttachmentNames() const {
        QStringList names;
        for (const Artifact &attachment : m_preparation.attachments)
            if (attachment.generatedByApp)
                names.append(attachment.displayName);
        for (const Artifact &exported : m_preparation.exports)
            if (exported.generatedByApp)
                names.append(exported.displayName);
        return names;
    }
    [[nodiscard]] QString draftIdentity() const { return m_lastResult.draftIdentity; }
    [[nodiscard]] QUrl presentationUrl() const { return m_lastResult.presentationUrl; }
    [[nodiscard]] const ValidationResult &issues() const { return m_issues; }
    [[nodiscard]] const EmailHandoffResult &lastResult() const { return m_lastResult; }
    [[nodiscard]] const EmailPreparation &preparation() const { return m_preparation; }

signals:
    void stateChanged();
    // The composition root owns the corresponding repository request and must
    // invalidate it immediately, rather than waiting for its worker callback.
    void sourceRevalidationCancelled();
    void handoffFinished(PN::Comm::EmailHandoffResult result);

private:
    bool startHandoff(bool sourceAlreadyRevalidated);
    EmailService *m_emailService = nullptr;
    EmailPreparation m_preparation;
    quint64 m_revision = 0;
    bool m_busy = false;
    PreparationStage m_stage = PreparationStage::Editing;
    ValidationResult m_issues;
    EmailHandoffResult m_lastResult;
    // A fingerprint mismatch is categorically different from a normal
    // recoverable backend error: the displayed document is no longer tied to
    // persisted source data and must be regenerated before dispatch.
    bool m_sourceRevalidationRequired = false;
};

} // namespace PN::Comm

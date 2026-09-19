// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "CommunicationsController.h"

#include "CommunicationDiagnostics.h"

#include <QCoreApplication>

#include <QPointer>
#include <QRegularExpression>

namespace PN::Comm {

CommunicationsController::CommunicationsController(EmailService *emailService, QObject *parent)
    : QObject(parent), m_emailService(emailService) {}

bool CommunicationsController::beginReviewPreparation()
{
    if (m_busy)
        return false;
    ++m_revision;
    m_preparation = {};
    m_issues = {};
    m_lastResult = {};
    m_sourceRevalidationRequired = false;
    m_stage = PreparationStage::Editing;
    emit stateChanged();
    return true;
}

QString CommunicationsController::stageName() const
{
    switch (m_stage) {
    case PreparationStage::Editing: return QStringLiteral("editing");
    case PreparationStage::Reviewing: return QStringLiteral("reviewing");
    case PreparationStage::RevalidatingSource: return QStringLiteral("revalidating-source");
    case PreparationStage::PreparingBackend: return QStringLiteral("preparing-backend");
    case PreparationStage::Completed: return QStringLiteral("completed");
    case PreparationStage::Failed: return QStringLiteral("failed");
    }
    return QStringLiteral("editing");
}

QString CommunicationsController::diagnostic() const
{
    if (m_issues.issues.isEmpty()) return {};
    const ValidationIssue &issue = m_issues.issues.constFirst();
    // Backends report a stable code with no prose. Showing the empty string
    // left the dialog in a failed state with nothing explaining why, so fall
    // back to the shared message table and finally to the bare code.
    if (!issue.displayText.isEmpty()) return issue.displayText;
    const QString mapped = diagnosticDisplayText(issue.code);
    if (!mapped.isEmpty()) return mapped;
    return QCoreApplication::translate("PN::Comm", "The email could not be prepared (%1).")
        .arg(issue.code);
}

bool CommunicationsController::setPreparation(EmailPreparation preparation)
{
    if (m_busy)
        return false;
    if (preparation.operationId.isNull())
        preparation.operationId = QUuid::createUuid();
    preparation.previewRevision = ++m_revision;
    m_preparation = std::move(preparation);
    m_issues = {};
    m_lastResult = {};
    m_sourceRevalidationRequired = false;
    m_stage = PreparationStage::Reviewing;
    emit stateChanged();
    return true;
}

void CommunicationsController::setReviewValidation(ValidationResult validation)
{
    if (m_busy)
        return;
    m_issues = std::move(validation);
    m_stage = m_issues.ok() ? PreparationStage::Reviewing : PreparationStage::Failed;
    emit stateChanged();
}

bool CommunicationsController::setSubject(QString subject)
{
    if (m_busy || (m_stage != PreparationStage::Reviewing && m_stage != PreparationStage::Failed) ||
        subject.contains('\r') || subject.contains('\n'))
        return false;
    if (m_preparation.document.defaultSubject == subject)
        return true;
    m_preparation.document.defaultSubject = std::move(subject);
    emit stateChanged();
    return true;
}

bool CommunicationsController::setHtmlBody(QString html)
{
    if (m_busy || (m_stage != PreparationStage::Reviewing && m_stage != PreparationStage::Failed)
        || m_preparation.mode != EmailMode::InlineHtml || html.trimmed().isEmpty())
        return false;
    if (m_preparation.document.emailFragment == html)
        return true;
    QString plain = html;
    plain.replace(QRegularExpression(QStringLiteral(R"(<\s*(br|/p|/div|/li)\s*/?\s*>)"),
                                     QRegularExpression::CaseInsensitiveOption), QStringLiteral("\n"));
    plain.remove(QRegularExpression(QStringLiteral(R"(<[^>]*>)")));
    m_preparation.document.emailFragment = std::move(html);
    m_preparation.document.plainText = plain.simplified();
    emit stateChanged();
    return true;
}

bool CommunicationsController::appendAttachment(Artifact attachment)
{
    if (m_busy || (m_stage != PreparationStage::Reviewing && m_stage != PreparationStage::Failed)
        || attachment.artifactId.isNull() || attachment.absolutePath.isEmpty())
        return false;
    m_preparation.attachments.append(std::move(attachment));
    emit stateChanged();
    return true;
}

bool CommunicationsController::handoff()
{
    if (m_busy || !m_emailService || m_sourceRevalidationRequired)
        return false;
    return startHandoff(false);
}

bool CommunicationsController::beginSourceRevalidation()
{
    if (m_busy || m_stage != PreparationStage::Reviewing)
        return false;
    m_busy = true;
    m_stage = PreparationStage::RevalidatingSource;
    emit stateChanged();
    return true;
}

bool CommunicationsController::handoffAfterSourceRevalidation()
{
    if (!m_busy || m_stage != PreparationStage::RevalidatingSource || !m_emailService)
        return false;
    return startHandoff(true);
}

bool CommunicationsController::handoffAfterSourceRevalidation(QList<EmailAddress> recipients, bool addressLater)
{
    if (!m_busy || m_stage != PreparationStage::RevalidatingSource || !m_emailService)
        return false;
    // Freeze the final selection in the same preparation that owns the edited
    // subject/body, generated attachments, and preview revision.
    m_preparation.recipients = std::move(recipients);
    m_preparation.addressLater = addressLater;
    return startHandoff(true);
}

void CommunicationsController::failSourceRevalidation(ValidationResult validation)
{
    if (m_stage != PreparationStage::RevalidatingSource)
        return;
    m_busy = false;
    m_issues = std::move(validation);
    m_stage = PreparationStage::Failed;
    m_sourceRevalidationRequired = true;
    emit stateChanged();
}

bool CommunicationsController::startHandoff(bool sourceAlreadyRevalidated)
{
    const EmailPreparationResult prepared = EmailContentBuilder::prepare(m_preparation);
    m_issues = prepared.validation;
    if (!prepared.validation.ok()) {
        m_busy = false;
        m_stage = PreparationStage::Failed;
        emit stateChanged();
        return false;
    }
    if (prepared.noEmail) {
        m_busy = false;
        m_stage = PreparationStage::Completed;
        emit stateChanged();
        return true;
    }
    const EmailRequest request = *prepared.request;
    const QUuid operationId = request.operationId;
    const quint64 revision = request.previewRevision;
    if (!sourceAlreadyRevalidated)
        m_busy = true;
    m_stage = PreparationStage::PreparingBackend;
    emit stateChanged();
    QPointer<CommunicationsController> self(this);
    m_emailService->handoff(request, [self, operationId, revision](EmailHandoffResult result) {
        if (!self || self->m_preparation.operationId != operationId || self->m_revision != revision)
            return;
        self->m_busy = false;
        self->m_lastResult = result;
        if (result.error.code.isEmpty()) self->m_stage = PreparationStage::Completed;
        else {
            self->m_stage = PreparationStage::Failed;
            self->m_issues = {};
            self->m_issues.addError(result.error.code, QStringLiteral("handoff"), result.error.displayText);
        }
        emit self->stateChanged();
        emit self->handoffFinished(std::move(result));
    });
    return true;
}

void CommunicationsController::cancel()
{
    if (m_stage == PreparationStage::RevalidatingSource) {
        m_busy = false;
        m_stage = PreparationStage::Reviewing;
        emit stateChanged();
        emit sourceRevalidationCancelled();
        return;
    }
    if (m_busy && m_emailService)
        m_emailService->cancel(m_preparation.operationId);
}

} // namespace PN::Comm

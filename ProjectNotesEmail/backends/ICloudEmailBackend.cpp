// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ICloudEmailBackend.h"

#include "ProjectNotesEmail/EmlWriter.h"

#include <QRegularExpression>
#include <QSslSocket>
#include <QTimer>

namespace PN::Comm {
namespace {

QByteArray imapQuoted(const QString &value)
{
    QByteArray bytes = value.toUtf8();
    bytes.replace('\\', "\\\\");
    bytes.replace('"', "\\\"");
    return '"' + bytes + '"';
}

bool taggedOk(const QByteArray &line, const QByteArray &tag)
{
    const QByteArray prefix = tag + " OK";
    return line.left(prefix.size()).compare(prefix, Qt::CaseInsensitive) == 0;
}

bool startsWithInsensitive(const QByteArray &line, const QByteArray &prefix)
{
    return line.left(prefix.size()).compare(prefix, Qt::CaseInsensitive) == 0;
}

} // namespace

struct ICloudEmailBackend::Session {
    enum class State { Greeting, Login, List, AppendContinuation, AppendResult };
    QUuid operationId;
    QSslSocket *socket = nullptr;
    QTimer *timer = nullptr;
    QByteArray input;
    QByteArray message;
    QString account;
    QString password;
    QString loginName;
    QString draftsMailbox = QStringLiteral("Drafts");
    Completion completion;
    State state = State::Greeting;
    bool literalSent = false;
    bool triedFullAccount = false;
};

ICloudEmailBackend::ICloudEmailBackend(ICloudCredentialsProvider credentials, QObject *parent)
    : QObject(parent), m_credentials(std::move(credentials))
{
}

ICloudEmailBackend::~ICloudEmailBackend()
{
    const auto ids = m_sessions.keys();
    for (const QUuid &id : ids)
        cancel(id);
}

void ICloudEmailBackend::handoff(EmailRequest request, Completion completion)
{
    EmailHandoffResult result;
    result.operationId = request.operationId;
    if (m_sessions.contains(request.operationId)) {
        result.error = {QStringLiteral("icloud-mail-operation-busy"), {}, RetryKind::ReviewAndRetry,
                        OutcomeCertainty::Certain};
        completion(result);
        return;
    }
    const ICloudCredentials credentials = m_credentials ? m_credentials() : ICloudCredentials{};
    if (!credentials.valid()) {
        result.error = {QStringLiteral("icloud-credentials-required"),
                        QStringLiteral("Enter your Apple Account and save an app-specific password in Settings."),
                        RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(result);
        return;
    }
    if (credentials.account.contains('\r') || credentials.account.contains('\n')
        || credentials.mailUser.contains('\r') || credentials.mailUser.contains('\n')
        || credentials.password.contains('\r') || credentials.password.contains('\n')) {
        result.error = {QStringLiteral("icloud-credentials-invalid"), {}, RetryKind::ReviewAndRetry,
                        OutcomeCertainty::Certain};
        completion(result);
        return;
    }

    ValidationResult validation;
    const QByteArray message = EmlWriter::write(request, {}, &validation);
    if (message.isEmpty()) {
        result.error = {validation.issues.isEmpty() ? QStringLiteral("eml-write-failed")
                                                    : validation.issues.constFirst().code,
                        {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        completion(result);
        return;
    }

    auto *session = new Session;
    session->operationId = request.operationId;
    session->message = message;
    session->account = credentials.mailUser.trimmed().isEmpty() ? credentials.account.trimmed()
                                                              : credentials.mailUser.trimmed();
    session->password = credentials.password;
    const QString domain = session->account.section('@', 1).toLower();
    session->loginName = QStringList{QStringLiteral("icloud.com"), QStringLiteral("me.com"),
                                     QStringLiteral("mac.com")}.contains(domain)
        ? session->account.section('@', 0, 0) : session->account;
    session->completion = std::move(completion);
    session->socket = new QSslSocket(this);
    session->timer = new QTimer(session->socket);
    session->timer->setSingleShot(true);
    session->timer->setInterval(30000);
    m_sessions.insert(session->operationId, session);

    const QUuid operationId = session->operationId;
    connect(session->timer, &QTimer::timeout, this, [this, operationId] {
        Session *active = m_sessions.value(operationId);
        if (active) fail(active, QStringLiteral("icloud-mail-timeout"), {}, active->literalSent);
    });
    connect(session->socket, &QSslSocket::readyRead, this, [this, operationId] {
        if (Session *active = m_sessions.value(operationId)) consume(active);
    });
    connect(session->socket, &QSslSocket::errorOccurred, this, [this, operationId](QAbstractSocket::SocketError) {
        Session *active = m_sessions.value(operationId);
        if (!active) return;
        fail(active, QStringLiteral("icloud-mail-network-error"), active->socket->errorString(),
             active->literalSent);
    });
    connect(session->socket, &QSslSocket::encrypted, this, [this, operationId] {
        if (Session *active = m_sessions.value(operationId)) active->timer->start();
    });

    // Apple documents SSL IMAP at imap.mail.me.com:993. APPEND creates the
    // server-side draft; Project Notes never connects to SMTP and cannot send.
    session->timer->start();
    session->socket->connectToHostEncrypted(QStringLiteral("imap.mail.me.com"), 993);
}

void ICloudEmailBackend::consume(Session *session)
{
    if (!m_sessions.contains(session->operationId)) return;
    session->timer->start();
    session->input += session->socket->readAll();
    while (true) {
        const qsizetype end = session->input.indexOf("\r\n");
        if (end < 0) return;
        const QByteArray line = session->input.left(end);
        session->input.remove(0, end + 2);

        if (session->state == Session::State::Greeting) {
            if (!startsWithInsensitive(line, "* OK")
                && !startsWithInsensitive(line, "* PREAUTH")) {
                fail(session, QStringLiteral("icloud-mail-protocol-error"));
                return;
            }
            if (startsWithInsensitive(line, "* PREAUTH")) {
                session->socket->write("A002 LIST \"\" \"*\"\r\n");
                session->state = Session::State::List;
            } else {
                const QByteArray account = imapQuoted(session->loginName);
                const QByteArray password = imapQuoted(session->password);
                session->socket->write("A001 LOGIN " + account + ' ' + password + "\r\n");
                session->state = Session::State::Login;
            }
            continue;
        }
        if (session->state == Session::State::Login) {
            if (!startsWithInsensitive(line, "A001 ")) continue;
            if (!taggedOk(line, "A001")) {
                // Apple documents the mailbox name as the usual username and
                // the full iCloud address as the fallback. Try both before
                // reporting an authentication failure.
                if (!session->triedFullAccount && session->loginName != session->account) {
                    session->triedFullAccount = true;
                    session->loginName = session->account;
                    session->socket->write("A001 LOGIN " + imapQuoted(session->loginName) + ' '
                                           + imapQuoted(session->password) + "\r\n");
                    continue;
                }
                fail(session, QStringLiteral("icloud-mail-authentication-failed"),
                     QStringLiteral("iCloud Mail rejected the Apple Account or app-specific password."));
                return;
            }
            session->password.clear();
            session->socket->write("A002 LIST \"\" \"*\"\r\n");
            session->state = Session::State::List;
            continue;
        }
        if (session->state == Session::State::List) {
            if (startsWithInsensitive(line, "* LIST ")
                && line.toLower().contains("\\drafts")) {
                const QRegularExpression quotedMailbox(QStringLiteral("\"((?:[^\"\\\\]|\\\\.)*)\"$"));
                const auto match = quotedMailbox.match(QString::fromUtf8(line));
                if (match.hasMatch()) {
                    QString mailbox = match.captured(1);
                    mailbox.replace(QStringLiteral("\\\""), QStringLiteral("\""));
                    mailbox.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
                    if (!mailbox.isEmpty()) session->draftsMailbox = mailbox;
                }
                continue;
            }
            if (!startsWithInsensitive(line, "A002 ")) continue;
            if (!taggedOk(line, "A002")) {
                fail(session, QStringLiteral("icloud-mail-drafts-unavailable"));
                return;
            }
            session->socket->write("A003 APPEND " + imapQuoted(session->draftsMailbox) + " (\\Draft) {"
                + QByteArray::number(session->message.size()) + "}\r\n");
            session->state = Session::State::AppendContinuation;
            continue;
        }
        if (session->state == Session::State::AppendContinuation) {
            if (line.startsWith('+')) {
                session->socket->write(session->message);
                session->socket->write("\r\n");
                session->literalSent = true;
                session->state = Session::State::AppendResult;
                continue;
            }
            if (startsWithInsensitive(line, "A003 ")) {
                fail(session, QStringLiteral("icloud-mail-drafts-unavailable"));
                return;
            }
            continue;
        }
        if (!startsWithInsensitive(line, "A003 ")) continue;
        if (!taggedOk(line, "A003")) {
            fail(session, QStringLiteral("icloud-mail-draft-create-failed"), {}, true);
            return;
        }
        EmailHandoffResult result;
        result.operationId = session->operationId;
        result.certainty = OutcomeCertainty::Certain;
        const QRegularExpression appendUid(QStringLiteral("\\[APPENDUID\\s+\\d+\\s+(\\d+)\\]"),
                                           QRegularExpression::CaseInsensitiveOption);
        const auto match = appendUid.match(QString::fromUtf8(line));
        result.draftIdentity = match.hasMatch() ? match.captured(1) : QStringLiteral("Drafts");
        // IMAP returns an identity, not an iCloud web URL for the new draft.
        finish(session, std::move(result));
        return;
    }
}

void ICloudEmailBackend::cancel(const QUuid &operationId)
{
    auto *session = m_sessions.value(operationId);
    if (!session) return;
    EmailHandoffResult result;
    result.operationId = operationId;
    result.certainty = session->literalSent ? OutcomeCertainty::Uncertain : OutcomeCertainty::Certain;
    result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, result.certainty};
    finish(session, std::move(result));
}

void ICloudEmailBackend::fail(Session *session, QString code, QString displayText, bool uncertain)
{
    EmailHandoffResult result;
    result.operationId = session->operationId;
    result.certainty = uncertain ? OutcomeCertainty::Uncertain : OutcomeCertainty::Certain;
    result.error = {std::move(code), std::move(displayText),
                    uncertain ? RetryKind::None : RetryKind::ReviewAndRetry, result.certainty};
    finish(session, std::move(result));
}

void ICloudEmailBackend::finish(Session *session, EmailHandoffResult result)
{
    if (!session || m_sessions.take(session->operationId) != session) return;
    session->timer->stop();
    session->socket->abort();
    session->socket->deleteLater();
    auto completion = std::move(session->completion);
    delete session;
    completion(std::move(result));
}

} // namespace PN::Comm

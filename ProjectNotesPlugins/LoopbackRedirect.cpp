// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "LoopbackRedirect.h"

#include <QHostAddress>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>

namespace PN::Plugins {

namespace {

// The page the browser shows once it has come back to Project Notes.
// serviceName (already HTML-escaped) names the service in the subtitle and
// message; without one the page just says the sign-in finished.
QByteArray donePage(const QString& serviceName)
{
    const QString subtitle = serviceName.isEmpty() ? QStringLiteral("Sign-in")
                                                   : serviceName + QStringLiteral(" Integration");
    const QString message = serviceName.isEmpty()
        ? QStringLiteral("You are now signed in.")
        : QStringLiteral("You are now authenticated with %1.").arg(serviceName);
    QString page = QStringLiteral(R"PAGE(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Project Notes &mdash; Login</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body {
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    background: linear-gradient(160deg, #1a2744 0%, #2a3f6f 60%, #1e3a5f 100%);
    font-family: 'Segoe UI', Arial, sans-serif;
  }
  .card {
    background: rgba(255,255,255,0.07);
    border: 1px solid rgba(255,255,255,0.15);
    border-radius: 12px;
    padding: 48px 56px;
    max-width: 460px;
    width: 90%;
    text-align: center;
    backdrop-filter: blur(4px);
  }
  .logo { margin-bottom: 28px; }
  .logo svg { width: 64px; height: 64px; filter: drop-shadow(0 2px 8px rgba(0,0,0,0.4)); }
  .app-name { color: #ffffff; font-size: 22px; font-weight: 600; letter-spacing: 0.5px; margin-bottom: 6px; }
  .app-sub {
    color: rgba(255,255,255,0.55);
    font-size: 13px;
    letter-spacing: 1.5px;
    text-transform: uppercase;
    margin-bottom: 36px;
  }
  .check-circle {
    width: 64px;
    height: 64px;
    background: linear-gradient(135deg, #2ecc71, #27ae60);
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    margin: 0 auto 24px;
    box-shadow: 0 4px 16px rgba(46,204,113,0.4);
    animation: pop 0.4s cubic-bezier(0.175,0.885,0.32,1.275);
  }
  @keyframes pop {
    from { transform: scale(0); opacity: 0; }
    to   { transform: scale(1); opacity: 1; }
  }
  .check-circle svg { width: 32px; height: 32px; }
  h1 { color: #ffffff; font-size: 20px; font-weight: 600; margin-bottom: 12px; }
  p { color: rgba(255,255,255,0.7); font-size: 14px; line-height: 1.6; }
  .divider { border: none; border-top: 1px solid rgba(255,255,255,0.12); margin: 32px 0 24px; }
  .footer { color: rgba(255,255,255,0.3); font-size: 11px; }
</style>
</head>
<body>
<div class="card">
  <div class="logo">
    <svg viewBox="0 0 64 64" fill="none" xmlns="http://www.w3.org/2000/svg">
      <rect x="10" y="14" width="44" height="46" rx="4" fill="rgba(255,255,255,0.15)" stroke="rgba(255,255,255,0.6)" stroke-width="2"/>
      <rect x="22" y="8" width="20" height="12" rx="3" fill="rgba(255,255,255,0.25)" stroke="rgba(255,255,255,0.7)" stroke-width="2"/>
      <line x1="18" y1="30" x2="46" y2="30" stroke="rgba(255,255,255,0.6)" stroke-width="2" stroke-linecap="round"/>
      <line x1="18" y1="38" x2="46" y2="38" stroke="rgba(255,255,255,0.6)" stroke-width="2" stroke-linecap="round"/>
      <line x1="18" y1="46" x2="36" y2="46" stroke="rgba(255,255,255,0.6)" stroke-width="2" stroke-linecap="round"/>
    </svg>
  </div>
  <div class="app-name">Project Notes</div>
  <div class="app-sub">%1</div>
  <div class="check-circle">
    <svg viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
      <polyline points="6,17 13,24 26,9" stroke="white" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
    </svg>
  </div>
  <h1>Login Successful</h1>
  <p>%2<br>You can close this window and return to Project Notes.</p>
  <hr class="divider">
  <div class="footer">This window was opened automatically by Project Notes.</div>
</div>
</body>
</html>)PAGE");
    return page.arg(subtitle, message).toUtf8();
}

} // namespace

LoopbackRedirect::LoopbackRedirect(QObject* parent)
    : QObject(parent)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (m_done)
            return;
        finish();
        emit failed(QStringLiteral("The browser sign-in did not finish in time."));
    });
}

LoopbackRedirect::~LoopbackRedirect()
{
    finish();
}

bool LoopbackRedirect::validRedirectUri(const QUrl& uri, QString* error)
{
    const QString host = uri.host().toLower();
    if (!uri.isValid() || uri.scheme() != QLatin1String("http")
        || (host != QLatin1String("localhost") && host != QLatin1String("127.0.0.1")) || uri.port() <= 0) {
        *error = QStringLiteral("redirectUri must be http://localhost:<port>/... or http://127.0.0.1:<port>/...");
        return false;
    }
    return true;
}

bool LoopbackRedirect::start(const QUrl& redirectUri, int timeoutMs, QString* error)
{
    if (!validRedirectUri(redirectUri, error))
        return false;
    m_redirectUri = redirectUri;
    const quint16 port = quint16(redirectUri.port());
    // "localhost" can resolve to either loopback address, so listen on both.
    const QList<QHostAddress> addresses = redirectUri.host() == QLatin1String("127.0.0.1")
        ? QList<QHostAddress>{QHostAddress(QHostAddress::LocalHost)}
        : QList<QHostAddress>{QHostAddress(QHostAddress::LocalHost), QHostAddress(QHostAddress::LocalHostIPv6)};
    QString lastError;
    for (const QHostAddress& address : addresses) {
        auto* server = new QTcpServer(this);
        if (!server->listen(address, port)) {
            lastError = server->errorString();
            delete server;
            continue;
        }
        connect(server, &QTcpServer::newConnection, this, [this, server] { handleConnection(server); });
        m_servers.append(server);
    }
    if (m_servers.isEmpty()) {
        *error = QStringLiteral("Could not listen on port %1 for the sign-in: %2").arg(port).arg(lastError);
        return false;
    }
    m_timeout.start(qMax(1000, timeoutMs));
    return true;
}

quint16 LoopbackRedirect::port() const
{
    return m_servers.isEmpty() ? 0 : m_servers.first()->serverPort();
}

void LoopbackRedirect::handleConnection(QTcpServer* server)
{
    while (QTcpSocket* socket = server->nextPendingConnection()) {
        // Owned by nobody: it lives until its reply is flushed and it
        // disconnects, even when this object and the server are gone.
        socket->setParent(nullptr);
        QPointer<QTcpSocket> guard(socket);
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        QTimer::singleShot(30000, socket, &QObject::deleteLater);
        connect(socket, &QTcpSocket::readyRead, this, [this, guard] {
            if (!guard)
                return;
            QTcpSocket* s = guard.data();
            const QByteArray head = s->peek(16384);
            if (!head.contains("\r\n\r\n") && head.size() < 16384)
                return;   // wait for the whole request head
            s->readAll();
            const QList<QByteArray> requestLine = head.left(head.indexOf("\r\n")).split(' ');
            const QUrl target = requestLine.size() >= 2 ? QUrl(QString::fromLatin1(requestLine.at(1))) : QUrl();
            const bool matches = !m_done && requestLine.value(0) == "GET"
                              && target.path() == (m_redirectUri.path().isEmpty() ? QStringLiteral("/") : m_redirectUri.path());
            if (!matches) {
                s->write("HTTP/1.1 204 No Content\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
                s->disconnectFromHost();
                return;
            }
            const QByteArray page = donePage(m_serviceName.toHtmlEscaped());
            s->write("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\nContent-Length: "
                     + QByteArray::number(page.size()) + "\r\n\r\n" + page);
            s->disconnectFromHost();
            QUrl callback = m_redirectUri;
            callback.setPath(target.path());
            callback.setQuery(target.query(QUrl::FullyEncoded), QUrl::StrictMode);
            finish();
            emit received(callback);
        });
    }
}

void LoopbackRedirect::finish()
{
    m_done = true;
    m_timeout.stop();
    for (QTcpServer* server : std::as_const(m_servers)) {
        server->close();
        server->deleteLater();
    }
    m_servers.clear();
}

} // namespace PN::Plugins

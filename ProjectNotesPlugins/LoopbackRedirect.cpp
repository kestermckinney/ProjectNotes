// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "LoopbackRedirect.h"

#include <QHostAddress>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>

namespace PN::Plugins {

namespace {

const char kDonePage[] =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Project Notes</title>"
    "<style>body{font-family:sans-serif;display:flex;align-items:center;justify-content:center;"
    "min-height:90vh;background:#1e2a44;color:#fff}div{text-align:center}p{opacity:.7}</style></head>"
    "<body><div><h2>You are signed in</h2><p>You can close this window and return to Project Notes.</p></div></body></html>";

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
            const QByteArray page(kDonePage);
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

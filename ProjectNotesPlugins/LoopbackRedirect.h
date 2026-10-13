// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <QObject>
#include <QTimer>
#include <QUrl>

class QTcpServer;

namespace PN::Plugins {

// Waits for one browser redirect on a loopback address (the last step of an
// OAuth authorization-code sign-in) and reports the full callback URL. Only
// http://localhost or http://127.0.0.1 redirect URIs are accepted, and the
// listener never binds to a non-loopback interface.
class LoopbackRedirect final : public QObject {
    Q_OBJECT
public:
    explicit LoopbackRedirect(QObject* parent = nullptr);
    ~LoopbackRedirect() override;

    static bool validRedirectUri(const QUrl& uri, QString* error);

    // Starts listening; received() or failed() follows, once.
    bool start(const QUrl& redirectUri, int timeoutMs, QString* error);
    quint16 port() const;

signals:
    void received(const QUrl& callbackUrl);
    void failed(const QString& message);

private:
    void handleConnection(QTcpServer* server);
    void finish();

    QUrl m_redirectUri;
    QList<QTcpServer*> m_servers;
    QTimer m_timeout;
    bool m_done = false;
};

} // namespace PN::Plugins

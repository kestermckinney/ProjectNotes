// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectNotesIntegrations/QtNetworkHttpTransport.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest/QtTest>

using namespace PN::Comm;

class QtNetworkHttpTransportTest final : public QObject
{
    Q_OBJECT
private slots:
    void sendsExactRequestAndResponse();
    void cancellationSuppressesStaleCompletion();
    void manualRedirectKeepsAuthorizationAtOriginalHost();
};

void QtNetworkHttpTransportTest::sendsExactRequestAndResponse()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost))
        QSKIP("The current sandbox forbids loopback listeners; run this local transport check on a normal host.");
    QByteArray received;
    connect(&server, &QTcpServer::newConnection, this, [&] {
        QTcpSocket *socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket, &received] {
            received += socket->readAll();
            if (received.contains("\r\n\r\n")) {
                socket->write("HTTP/1.1 201 Created\r\nX-Synthetic: yes\r\nContent-Length: 2\r\n\r\nok");
                socket->disconnectFromHost();
            }
        });
    });

    QtNetworkHttpTransport transport(nullptr, 1000);
    HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = "PATCH";
    request.url = QUrl(QStringLiteral("http://127.0.0.1:%1/v1/draft").arg(server.serverPort()));
    request.headers.insert("X-Request", "synthetic");
    request.body = "payload";
    bool completed = false;
    HttpResponse response;
    transport.send(request, [&response, &completed](HttpResponse value) {
        response = std::move(value);
        completed = true;
    });

    QTRY_VERIFY(completed);
    QCOMPARE(response.operationId, request.operationId);
    QCOMPARE(response.statusCode, 201);
    QByteArray synthetic;
    for (auto it = response.headers.cbegin(); it != response.headers.cend(); ++it)
        if (it.key().compare("X-Synthetic", Qt::CaseInsensitive) == 0)
            synthetic = it.value();
    QCOMPARE(synthetic, QByteArray("yes"));
    QCOMPARE(response.body, QByteArray("ok"));
    QVERIFY(response.error.code.isEmpty());
    QVERIFY(received.startsWith("PATCH /v1/draft HTTP/1.1\r\n"));
    QVERIFY(received.toLower().contains("x-request: synthetic\r\n"));
    QVERIFY(received.endsWith("payload"));
}

void QtNetworkHttpTransportTest::cancellationSuppressesStaleCompletion()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost))
        QSKIP("The current sandbox forbids loopback listeners; run this local transport check on a normal host.");
    connect(&server, &QTcpServer::newConnection, this, [&] {
        QTcpSocket *socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket] { socket->readAll(); });
    });

    QtNetworkHttpTransport transport(nullptr, 1000);
    HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = "GET";
    request.url = QUrl(QStringLiteral("http://127.0.0.1:%1/pending").arg(server.serverPort()));
    bool completed = false;
    transport.send(request, [&completed](HttpResponse) { completed = true; });
    transport.cancel(request.operationId);
    QTest::qWait(50);
    QVERIFY(!completed);
}

void QtNetworkHttpTransportTest::manualRedirectKeepsAuthorizationAtOriginalHost()
{
    QTcpServer origin, destination;
    if (!origin.listen(QHostAddress::LocalHost) || !destination.listen(QHostAddress::LocalHost))
        QSKIP("Loopback listeners are unavailable in this sandbox.");
    const QByteArray location = "http://127.0.0.1:" + QByteArray::number(destination.serverPort()) + "/redirected";
    QSignalSpy redirected(&destination, &QTcpServer::newConnection);
    connect(&origin, &QTcpServer::newConnection, this, [&] {
        auto *socket = origin.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket, location] {
            socket->readAll();
            socket->write("HTTP/1.1 302 Found\r\nLocation: " + location + "\r\nContent-Length: 0\r\n\r\n");
            socket->disconnectFromHost();
        });
    });
    QtNetworkHttpTransport transport;
    HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = "GET";
    request.url = QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(origin.serverPort()));
    request.headers.insert("Authorization", "Basic synthetic");
    request.manualRedirects = true;
    bool completed = false;
    HttpResponse response;
    transport.send(request, [&](HttpResponse result) { response = std::move(result); completed = true; });
    QTRY_VERIFY(completed);
    QCOMPARE(response.statusCode, 302);
    QCOMPARE(redirected.count(), 0);
}

QTEST_MAIN(QtNetworkHttpTransportTest)
#include "tst_httptransport.moc"

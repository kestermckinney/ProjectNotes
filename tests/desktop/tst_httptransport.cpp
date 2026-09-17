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
    QCOMPARE(response.headers.value("X-Synthetic"), QByteArray("yes"));
    QCOMPARE(response.body, QByteArray("ok"));
    QVERIFY(response.error.code.isEmpty());
    QVERIFY(received.startsWith("PATCH /v1/draft HTTP/1.1\r\n"));
    QVERIFY(received.contains("X-Request: synthetic\r\n"));
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

QTEST_MAIN(QtNetworkHttpTransportTest)
#include "tst_httptransport.moc"

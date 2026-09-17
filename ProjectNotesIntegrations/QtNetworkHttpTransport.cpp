// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "QtNetworkHttpTransport.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace PN::Comm {

QtNetworkHttpTransport::QtNetworkHttpTransport(QObject *parent, int timeoutMs)
    : QObject(parent), m_network(new QNetworkAccessManager(this)), m_timeoutMs(qMax(1, timeoutMs))
{
}

void QtNetworkHttpTransport::send(HttpRequest request, Completion completion)
{
    // A request ID identifies one in-flight operation.  Retiring an earlier
    // reply makes a reused ID deterministic instead of allowing a stale reply
    // to complete a newer operation.
    cancel(request.operationId);

    QNetworkRequest networkRequest(request.url);
    for (auto it = request.headers.cbegin(); it != request.headers.cend(); ++it)
        networkRequest.setRawHeader(it.key(), it.value());

    QNetworkReply *reply = nullptr;
    if (request.method == "GET")
        reply = m_network->get(networkRequest);
    else if (request.method == "POST")
        reply = m_network->post(networkRequest, request.body);
    else if (request.method == "PUT")
        reply = m_network->put(networkRequest, request.body);
    else if (request.method == "DELETE")
        reply = m_network->deleteResource(networkRequest);
    else
        reply = m_network->sendCustomRequest(networkRequest, request.method, request.body);

    const QUuid operationId = request.operationId;
    m_pending.insert(operationId, {reply, false});
    QTimer::singleShot(m_timeoutMs, reply, [this, operationId, reply] {
        auto it = m_pending.find(operationId);
        if (it != m_pending.end() && it->reply == reply && reply->isRunning()) {
            it->timedOut = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this,
            [this, operationId, reply, completion = std::move(completion)]() mutable {
        auto it = m_pending.find(operationId);
        if (it == m_pending.end() || it->reply != reply) {
            reply->deleteLater();
            return;
        }
        const bool timedOut = it->timedOut;
        m_pending.erase(it);

        HttpResponse response;
        response.operationId = operationId;
        response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto headers = reply->rawHeaderPairs();
        for (const auto &header : headers)
            response.headers.insert(header.first, header.second);
        response.body = reply->readAll();
        if (timedOut)
            response.error = {QStringLiteral("http-timeout"), tr("The network request timed out.")};
        else if (reply->error() != QNetworkReply::NoError)
            response.error = {QStringLiteral("http-network-error"), reply->errorString()};
        reply->deleteLater();
        completion(std::move(response));
    });
}

void QtNetworkHttpTransport::cancel(const QUuid &operationId)
{
    const auto it = m_pending.find(operationId);
    if (it == m_pending.end())
        return;
    QPointer<QNetworkReply> reply = it->reply;
    m_pending.erase(it);
    if (reply && reply->isRunning())
        reply->abort();
}

} // namespace PN::Comm

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ArtifactStore.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QNetworkReply>
#include <functional>

namespace PN::Comm {
// Streams downloads into the operation-owned cache, which is removed on exit.
class LocationAttachmentDownloader final : public QObject {
public:
    using Completion = std::function<void(ArtifactStoreResult)>;
    explicit LocationAttachmentDownloader(QObject *parent = nullptr) : QObject(parent), m_network(this) {}
    void download(const QUrl &url, const QString &cacheDirectory, const QUuid &operationId, Completion completion);
    void cancel();
private:
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
};
}

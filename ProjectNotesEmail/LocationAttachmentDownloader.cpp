// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "LocationAttachmentDownloader.h"
#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>
#include <memory>

namespace PN::Comm {
namespace {
QString safeFileName(QString name)
{
    name.replace(u'\\', u'/');
    name = QFileInfo(name).fileName();
    for (QChar &character : name)
        if (!character.isPrint() || QStringLiteral("/\\:*?\"<>|").contains(character)) character = u'_';
    while (name.contains(QStringLiteral(".."))) name.replace(QStringLiteral(".."), QStringLiteral("_"));
    if (name.trimmed().isEmpty() || name == ".") name = QStringLiteral("download");
    return name.left(180);
}
}
void LocationAttachmentDownloader::cancel()
{
    if (m_reply) {
        auto reply = m_reply;
        m_reply = nullptr;
        reply->abort();
    }
}

void LocationAttachmentDownloader::download(const QUrl &url, const QString &cacheDirectory,
                                           const QUuid &operationId, Completion completion)
{
    cancel();
    if (!url.isValid() || url.host().isEmpty() || (url.scheme() != "http" && url.scheme() != "https")) {
        completion({{}, {QStringLiteral("location-url-invalid"), QStringLiteral("Choose an HTTP/HTTPS URL to download.")}});
        return;
    }
    ArtifactStore store(cacheDirectory);
    ServiceError error;
    const QString path = store.reserveGeneratedPath(operationId, QStringLiteral("_location-download"), &error);
    auto file = std::make_shared<QSaveFile>(path);
    if (path.isEmpty() || !file->open(QIODevice::WriteOnly)) {
        completion({{}, {QStringLiteral("location-download-write-failed"), QStringLiteral("The attachment could not be saved in the program cache.")}});
        return;
    }
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(60000);
    auto reply = m_network.get(request);
    m_reply = reply;
    reply->setReadBufferSize(256 * 1024);
    auto received = std::make_shared<qint64>(0);
    auto failure = std::make_shared<QString>();
    const auto drain = [reply, file, received, failure] {
        const QByteArray bytes = reply->readAll();
        *received += bytes.size();
        if (*received > 50 * 1024 * 1024) {
            *failure = QStringLiteral("The download exceeds the 50 MB attachment limit.");
            reply->abort();
        } else if (file->write(bytes) != bytes.size()) {
            *failure = QStringLiteral("The attachment could not be saved in the program cache.");
            reply->abort();
        }
    };
    connect(reply, &QNetworkReply::readyRead, this, drain);
    QTimer::singleShot(60000, reply, [reply] { if (!reply->isFinished()) reply->abort(); });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, file, failure, drain, cacheDirectory, operationId, path,
             completion = std::move(completion)]() mutable {
        const bool current = m_reply == reply;
        if (current) m_reply = nullptr;
        if (reply->bytesAvailable()) drain();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        ArtifactStoreResult result;
        if (!current || !failure->isEmpty() || reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
            file->cancelWriting();
            result.error = {QStringLiteral("location-download-failed"), !failure->isEmpty() ? *failure : reply->errorString()};
        } else if (!file->commit()) {
            result.error = {QStringLiteral("location-download-write-failed"), file->errorString()};
        } else {
            QString displayName = safeFileName(QFileInfo(reply->url().path()).fileName());
            const QString disposition = QString::fromUtf8(reply->rawHeader("Content-Disposition"));
            static const QRegularExpression encoded(QStringLiteral("filename\\*\\s*=\\s*UTF-8''([^;]+)"), QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression plain(QStringLiteral("filename\\s*=\\s*(?:\"([^\"]+)\"|([^;]+))"), QRegularExpression::CaseInsensitiveOption);
            const auto encodedMatch = encoded.match(disposition);
            const auto plainMatch = plain.match(disposition);
            if (encodedMatch.hasMatch()) displayName = QUrl::fromPercentEncoding(encodedMatch.captured(1).trimmed().toUtf8());
            else if (plainMatch.hasMatch()) displayName = plainMatch.captured(1).isEmpty() ? plainMatch.captured(2).trimmed() : plainMatch.captured(1);
            displayName = safeFileName(displayName);
            ArtifactStore store(cacheDirectory);
            ServiceError error;
            const QString finalPath = store.reserveGeneratedPath(operationId, displayName, &error);
            if (finalPath.isEmpty() || !QFile::rename(path, finalPath)) {
                result.error = {QStringLiteral("location-download-write-failed"), QStringLiteral("The downloaded attachment could not be saved.")};
            } else {
                QString mime = reply->header(QNetworkRequest::ContentTypeHeader).toString().section(';', 0, 0).trimmed();
                if (mime.isEmpty() || mime == "application/octet-stream") mime = QMimeDatabase().mimeTypeForFile(finalPath).name();
                result = store.registerGeneratedFile(operationId, finalPath, displayName, mime);
            }
        }
        reply->deleteLater();
        if (current) completion(std::move(result));
    });
}
}

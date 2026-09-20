// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "EmlWriter.h"

#include <QFile>
#include <QFileInfo>
#include <QLocale>

namespace PN::Comm {
namespace {

bool containsLineBreak(const QString &value) { return value.contains('\r') || value.contains('\n'); }

bool isAscii(const QString &value)
{
    for (const QChar character : value)
        if (character.unicode() > 127) return false;
    return true;
}

// Wrapped at 76 characters, the limit RFC 2045 sets for a base64 body.
QByteArray base64Lines(const QByteArray &content)
{
    const QByteArray encoded = content.toBase64();
    QByteArray wrapped;
    for (qsizetype offset = 0; offset < encoded.size(); offset += 76)
        wrapped += encoded.mid(offset, 76) + "\r\n";
    return wrapped;
}

// Header text outside US-ASCII travels as an RFC 2047 encoded word.  Encoding
// the whole value (rather than per word) keeps this short and is what mail
// clients produce themselves.
QByteArray headerText(const QString &value)
{
    if (isAscii(value)) return value.toLatin1();
    return "=?UTF-8?B?" + value.toUtf8().toBase64() + "?=";
}

QByteArray addressHeader(const QString &displayName, const QString &address)
{
    const QString name = displayName.trimmed();
    if (name.isEmpty()) return address.trimmed().toLatin1();
    if (isAscii(name)) return '"' + QString(name).remove('"').toLatin1() + "\" <" + address.trimmed().toLatin1() + '>';
    return headerText(name) + " <" + address.trimmed().toLatin1() + '>';
}

QByteArray fileName(const QString &displayName, const QString &path)
{
    const QString name = displayName.trimmed().isEmpty() ? QFileInfo(path).fileName() : displayName.trimmed();
    return headerText(QString(name).remove('"').remove('\r').remove('\n'));
}

} // namespace

QByteArray EmlWriter::write(const EmailRequest &request, const QDateTime &stamp, ValidationResult *validation)
{
    auto error = [validation](const QString &code, const QString &field) {
        if (validation) validation->addError(code, field);
    };
    if (containsLineBreak(request.subject)) { error("eml-header-injection", "subject"); return {}; }

    QByteArray to, cc, bcc;
    for (const EmailAddress &recipient : request.recipients) {
        const QString address = recipient.address.trimmed();
        if (address.isEmpty()) continue;
        if (containsLineBreak(address) || address.contains(',') || address.contains(';') || !address.contains('@')) {
            error("recipient-address-invalid", "recipients"); return {};
        }
        if (containsLineBreak(recipient.displayName)) { error("eml-header-injection", "recipients"); return {}; }
        QByteArray &target = recipient.role == RecipientRole::To ? to : (recipient.role == RecipientRole::Cc ? cc : bcc);
        if (!target.isEmpty()) target += ", ";
        target += addressHeader(recipient.displayName, address);
    }
    if (to.isEmpty() && cc.isEmpty() && bcc.isEmpty() && !request.addressLaterExplicitlyChosen) {
        error("recipient-required", "recipients"); return {};
    }

    // Read the attachments before writing anything: a message naming a file it
    // could not read would be handed over looking complete.
    struct Part { QByteArray headers; QByteArray content; };
    QList<Part> parts;
    for (const Artifact &attachment : request.attachments) {
        QFile file(attachment.absolutePath);
        if (!file.open(QIODevice::ReadOnly)) { error("eml-attachment-unreadable", "attachments"); return {}; }
        const QByteArray content = file.readAll();
        if (file.error() != QFile::NoError) { error("eml-attachment-unreadable", "attachments"); return {}; }
        const QByteArray name = fileName(attachment.displayName, attachment.absolutePath);
        const QByteArray mimeType = attachment.mimeType.trimmed().isEmpty()
            ? QByteArray("application/octet-stream") : attachment.mimeType.trimmed().toLatin1();
        QByteArray headers;
        headers += "Content-Type: " + mimeType + ";\r\n\tname=\"" + name + "\"\r\n";
        headers += "Content-Disposition: attachment; filename=\"" + name + "\"\r\n";
        headers += "Content-Transfer-Encoding: base64\r\n";
        parts.append({headers, content});
    }

    const QDateTime sent = stamp.isValid() ? stamp : QDateTime::currentDateTime();
    // A C-locale format: the month and day names in a Date header are fixed by
    // RFC 5322 and must not follow the user's locale.
    const QByteArray date = QLocale::c().toString(sent.toUTC(), QStringLiteral("ddd, dd MMM yyyy HH:mm:ss"))
                                .toLatin1() + " +0000";
    const QByteArray boundary = "=_ProjectNotes_"
        + request.operationId.toString(QUuid::WithoutBraces).remove('-').toLatin1() + "_";

    QByteArray message;
    message += "Date: " + date + "\r\n";
    if (!request.subject.isEmpty()) message += "Subject: " + headerText(request.subject) + "\r\n";
    if (!to.isEmpty()) message += "To: " + to + "\r\n";
    if (!cc.isEmpty()) message += "Cc: " + cc + "\r\n";
    if (!bcc.isEmpty()) message += "Bcc: " + bcc + "\r\n";
    // Outlook opens a message carrying X-Unsent as a new, editable draft rather
    // than as received mail; clients that don't know the header ignore it.
    message += "X-Unsent: 1\r\n";
    message += "MIME-Version: 1.0\r\n";

    const QByteArray body = base64Lines(request.html.toUtf8());
    if (parts.isEmpty()) {
        message += "Content-Type: text/html; charset=\"utf-8\"\r\n";
        message += "Content-Transfer-Encoding: base64\r\n\r\n";
        message += body;
        return message;
    }

    message += "Content-Type: multipart/mixed;\r\n\tboundary=\"" + boundary + "\"\r\n\r\n";
    message += "--" + boundary + "\r\n";
    message += "Content-Type: text/html; charset=\"utf-8\"\r\n";
    message += "Content-Transfer-Encoding: base64\r\n\r\n";
    message += body;
    for (const Part &part : parts) {
        message += "\r\n--" + boundary + "\r\n";
        message += part.headers + "\r\n";
        message += base64Lines(part.content);
    }
    message += "\r\n--" + boundary + "--\r\n";
    return message;
}

} // namespace PN::Comm

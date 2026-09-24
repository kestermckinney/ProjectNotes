// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "IcsWriter.h"

namespace PN::Comm {
namespace {
// A parameter value is quoted and may not contain DQUOTE or control characters.
QString parameterValue(const QString &value)
{
    QString result;
    for (const QChar character : value)
        if (character != u'"' && character.unicode() >= 0x20 && character.unicode() != 0x7f)
            result.append(character);
    return QLatin1Char('"') + result.trimmed() + QLatin1Char('"');
}
}

QString IcsWriter::escapeText(const QString &value)
{
    QString result;
    result.reserve(value.size());
    for (qsizetype index = 0; index < value.size(); ++index) {
        const QChar character = value.at(index);
        if (character == u'\\') result += QStringLiteral("\\\\");
        else if (character == u';') result += QStringLiteral("\\;");
        else if (character == u',') result += QStringLiteral("\\,");
        else if (character == u'\n') result += QStringLiteral("\\n");
        else if (character == u'\r') {
            if (index + 1 >= value.size() || value.at(index + 1) != u'\n') result += QStringLiteral("\\n");
        } else if (character.unicode() < 0x20 && character != u'\t') {
            continue;
        } else {
            result += character;
        }
    }
    return result;
}

QByteArray IcsWriter::foldLine(const QString &line)
{
    // Lines are limited to 75 octets; a continuation starts with one space,
    // which counts toward the limit.  Never split a UTF-8 sequence.
    const QByteArray bytes = line.toUtf8();
    QByteArray result;
    qsizetype lineLength = 0;
    for (qsizetype index = 0; index < bytes.size();) {
        qsizetype width = 1;
        const auto lead = static_cast<unsigned char>(bytes.at(index));
        if (lead >= 0xf0) width = 4;
        else if (lead >= 0xe0) width = 3;
        else if (lead >= 0xc0) width = 2;
        width = std::min(width, bytes.size() - index);
        if (lineLength + width > 75) {
            result += "\r\n ";
            lineLength = 1;
        }
        result += bytes.mid(index, width);
        lineLength += width;
        index += width;
    }
    result += "\r\n";
    return result;
}

QString IcsWriter::utcStamp(const QDateTime &value)
{
    return value.toUTC().toString(QStringLiteral("yyyyMMdd'T'HHmmss'Z'"));
}

QByteArray IcsWriter::write(const MeetingRequest &request, const QDateTime &stamp, ValidationResult *validation)
{
    const ValidationResult checked = validate(request);
    if (!checked.ok()) {
        if (validation) validation->issues += checked.issues;
        return {};
    }
    QStringList lines{
        QStringLiteral("BEGIN:VCALENDAR"),
        QStringLiteral("VERSION:2.0"),
        QStringLiteral("PRODID:-//Project Notes//Project Notes//EN"),
        QStringLiteral("CALSCALE:GREGORIAN"),
        QStringLiteral("METHOD:PUBLISH"),
        QStringLiteral("BEGIN:VEVENT"),
        QStringLiteral("UID:%1@projectnotes").arg(request.operationId.toString(QUuid::WithoutBraces)),
        QStringLiteral("DTSTAMP:") + utcStamp(stamp.isValid() ? stamp : QDateTime::currentDateTimeUtc()),
        QStringLiteral("DTSTART:") + utcStamp(request.start),
        QStringLiteral("DTEND:") + utcStamp(request.end),
        QStringLiteral("SEQUENCE:0"),
        QStringLiteral("SUMMARY:") + escapeText(request.subject.trimmed()),
    };
    if (!request.location.trimmed().isEmpty())
        lines.append(QStringLiteral("LOCATION:") + escapeText(request.location.trimmed()));
    if (!request.plainAgenda.trimmed().isEmpty())
        lines.append(QStringLiteral("DESCRIPTION:") + escapeText(request.plainAgenda));
    if (!request.htmlAgenda.trimmed().isEmpty())
        lines.append(QStringLiteral("X-ALT-DESC;FMTTYPE=text/html:") + escapeText(request.htmlAgenda));
    if (!request.organizerAddress.trimmed().isEmpty()) {
        QString organizer = QStringLiteral("ORGANIZER");
        if (!request.organizerName.trimmed().isEmpty())
            organizer += QStringLiteral(";CN=") + parameterValue(request.organizerName);
        lines.append(organizer + QStringLiteral(":mailto:") + request.organizerAddress.trimmed());
    }
    for (const MeetingAttendee &attendee : request.attendees) {
        QString line = QStringLiteral("ATTENDEE");
        if (!attendee.displayName.trimmed().isEmpty())
            line += QStringLiteral(";CN=") + parameterValue(attendee.displayName);
        line += attendee.role == AttendeeRole::Required ? QStringLiteral(";ROLE=REQ-PARTICIPANT")
                                                        : QStringLiteral(";ROLE=OPT-PARTICIPANT");
        line += QStringLiteral(";PARTSTAT=NEEDS-ACTION;RSVP=TRUE:mailto:") + attendee.address.trimmed();
        lines.append(line);
    }
    lines += {QStringLiteral("END:VEVENT"), QStringLiteral("END:VCALENDAR")};

    QByteArray result;
    for (const QString &line : lines)
        result += foldLine(line);
    return result;
}

} // namespace PN::Comm

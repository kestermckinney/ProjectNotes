// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "MeetingTypes.h"

#include <QByteArray>

namespace PN::Comm {

// Writes one meeting as an RFC 5545 iCalendar file.  Times are written in UTC
// so the file needs no VTIMEZONE block; the calendar app shows them in the
// user's own zone. Published files use METHOD:PUBLISH so calendar apps open
// them as events to save and send. CalDAV resources omit METHOD as RFC 4791
// requires.
class IcsWriter final {
public:
    enum class OutputKind {
        PublishedFile,
        CalDavResource
    };

    static QByteArray write(const MeetingRequest &request, const QDateTime &stamp,
                            ValidationResult *validation = nullptr,
                            OutputKind outputKind = OutputKind::PublishedFile);

    // Exposed for tests.
    static QString escapeText(const QString &value);
    static QByteArray foldLine(const QString &line);
    static QString utcStamp(const QDateTime &value);
};

} // namespace PN::Comm

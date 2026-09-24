// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "MeetingTypes.h"

#include <QByteArray>

namespace PN::Comm {

// Writes one meeting as an RFC 5545 iCalendar file.  Times are written in UTC
// so the file needs no VTIMEZONE block; the calendar app shows them in the
// user's own zone.  METHOD:PUBLISH makes calendar apps open the file as an
// event to save and send, rather than as an invitation received from someone.
class IcsWriter final {
public:
    static QByteArray write(const MeetingRequest &request, const QDateTime &stamp,
                            ValidationResult *validation = nullptr);

    // Exposed for tests.
    static QString escapeText(const QString &value);
    static QByteArray foldLine(const QString &line);
    static QString utcStamp(const QDateTime &value);
};

} // namespace PN::Comm

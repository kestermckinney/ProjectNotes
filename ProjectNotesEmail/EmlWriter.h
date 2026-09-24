// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include <QByteArray>
#include <QDateTime>

namespace PN::Comm {

// Builds an RFC 5322 message for the default mail application to open.  The
// message is always HTML: Project Notes no longer produces plain-text bodies.
// Attachments become base64 parts of a multipart/mixed message, which is what
// a mailto: URL could never carry.
class EmlWriter final {
public:
    // Returns an empty array and fills `validation` when the request cannot be
    // expressed as a message (header injection, unreadable attachment, or no
    // recipient when the user did not choose to address it later).
    static QByteArray write(const EmailRequest &request, const QDateTime &stamp = {},
                            ValidationResult *validation = nullptr);
};

} // namespace PN::Comm

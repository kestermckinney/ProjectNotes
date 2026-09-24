// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

namespace PN::Comm {

// Backends and validators report stable machine codes and deliberately carry no
// user-facing prose, so a failure that reaches the review dialog would otherwise
// render as an empty message. This maps a code to text a person can act on.
// Returns an empty string for codes that have no specific message yet; callers
// fall back to a generic message rather than showing nothing.
[[nodiscard]] QString diagnosticDisplayText(const QString &code);

} // namespace PN::Comm

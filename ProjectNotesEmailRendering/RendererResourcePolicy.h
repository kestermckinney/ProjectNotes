// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QUrl>

namespace PN::Comm {

// Render requests use setHtml() with an about:blank base URL. The document's
// own main-frame data/about load is allowed, but report/template HTML never
// receives network, file, qrc, or nested-frame resources.
bool rendererResourceRequestAllowed(const QUrl &url, bool isMainFrame);

} // namespace PN::Comm

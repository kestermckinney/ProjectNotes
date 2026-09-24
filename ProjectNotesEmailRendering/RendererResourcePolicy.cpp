// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "RendererResourcePolicy.h"

namespace PN::Comm {

bool rendererResourceRequestAllowed(const QUrl &url, bool isMainFrame)
{
    if (!isMainFrame)
        return false;
    return url.scheme() == QLatin1String("about") || url.scheme() == QLatin1String("data");
}

} // namespace PN::Comm

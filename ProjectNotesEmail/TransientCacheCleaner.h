// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>
#include <QStringList>

namespace PN::Comm {

// Removes the temporary files Project Notes keeps in its cache location.  It
// runs when the application quits and again at the next launch, which clears
// anything a crash left behind.
//
// Only an explicit allow-list is removed.  The same cache folder also holds
// state the Python plug-ins must keep between sessions (Microsoft 365 and IFS
// token caches, saved plug-in state), and nothing outside the list is touched.
class TransientCacheCleaner final {
public:
    explicit TransientCacheCleaner(QString cacheDirectory);

    // Deletes the communications staging tree (email attachments, rendered
    // reports, meeting .ics files) and the plug-ins' generated report files.
    // Returns the paths that could not be removed.
    QStringList clean() const;

    // File name patterns removed from the top level of the cache folder only.
    static QStringList transientFilePatterns();

private:
    QString m_cacheDirectory;
};

} // namespace PN::Comm

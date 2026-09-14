// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "officedeeplink.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

QString unwrapOfficeDeepLink(const QString& fullPath)
{
    static const QRegularExpression legacy(
        QStringLiteral("^ms-[a-z]+:of[ev]\\|u\\|(.+)$"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = legacy.match(fullPath);
    return match.hasMatch() ? match.captured(1) : fullPath;
}

namespace {

// Key off the URL's own path, never a query parameter: a viewer page such as
// Doc.aspx?file=Budget.xlsx names a workbook but isn't one, and Office can't
// open it. A direct document URL needs no query string, and the Office URI
// spec forbids "?" in the argument, so drop query and fragment. Returns the
// direct URL and, via suffixOut, its lowercased extension; an empty URL means
// fullPath isn't an http(s) URL at all.
QString directOfficeDocumentUrl(const QString& fullPath, QString* suffixOut)
{
    const QString url = unwrapOfficeDeepLink(fullPath.trimmed());
    if (!url.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
        && !url.startsWith(QLatin1String("https://"), Qt::CaseInsensitive))
        return QString();

    const QString direct = url.section(QLatin1Char('?'), 0, 0).section(QLatin1Char('#'), 0, 0);
    *suffixOut = QFileInfo(QUrl(direct, QUrl::TolerantMode).path()).suffix().toLower();
    return direct;
}

// Empty when suffix isn't a Word/Excel/PowerPoint/Project extension.
QString officeUriScheme(const QString& suffix)
{
    static const QStringList word = {"doc", "docx", "docm", "dot", "dotx", "dotm"};
    static const QStringList excel = {"xls", "xlsx", "xlsm", "xlsb", "xlt", "xltx", "xltm"};
    static const QStringList powerpoint = {"ppt", "pptx", "pptm", "pps", "ppsx", "pot", "potx"};
    static const QStringList project = {"mpp", "mpt"};

    if (word.contains(suffix))
        return QStringLiteral("ms-word:");
    if (excel.contains(suffix))
        return QStringLiteral("ms-excel:");
    if (powerpoint.contains(suffix))
        return QStringLiteral("ms-powerpoint:");
    if (project.contains(suffix))
        return QStringLiteral("ms-project:");
    return QString();
}

} // namespace

QString officeDeepLinkFor(const QString& fullPath)
{
    QString suffix;
    const QString direct = directOfficeDocumentUrl(fullPath, &suffix);
    if (direct.isEmpty())
        return QString();

    const QString scheme = officeUriScheme(suffix);
    if (scheme.isEmpty())
        return QString();

    return scheme + QStringLiteral("ofe|u|") + direct;
}

QString officeWebViewerUrlFor(const QString& fullPath)
{
    QString suffix;
    const QString direct = directOfficeDocumentUrl(fullPath, &suffix);
    if (direct.isEmpty() || officeUriScheme(suffix).isEmpty())
        return QString();

    return direct + QStringLiteral("?web=1");
}

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

QString officeDeepLinkFor(const QString& fullPath)
{
    const QString url = unwrapOfficeDeepLink(fullPath.trimmed());
    if (!url.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
        && !url.startsWith(QLatin1String("https://"), Qt::CaseInsensitive))
        return QString();

    // Key off the URL's own path, never a query parameter: a viewer page such
    // as Doc.aspx?file=Budget.xlsx names a workbook but isn't one, and Office
    // can't open it. A direct document URL needs no query string, and the
    // Office URI spec forbids "?" in the argument, so drop query and fragment.
    const QString direct = url.section(QLatin1Char('?'), 0, 0).section(QLatin1Char('#'), 0, 0);
    const QString suffix = QFileInfo(QUrl(direct, QUrl::TolerantMode).path()).suffix().toLower();

    static const QStringList word = {"doc", "docx", "docm", "dot", "dotx", "dotm"};
    static const QStringList excel = {"xls", "xlsx", "xlsm", "xlsb", "xlt", "xltx", "xltm"};
    static const QStringList powerpoint = {"ppt", "pptx", "pptm", "pps", "ppsx", "pot", "potx"};
    static const QStringList project = {"mpp", "mpt"};

    QString scheme;
    if (word.contains(suffix))
        scheme = QStringLiteral("ms-word:");
    else if (excel.contains(suffix))
        scheme = QStringLiteral("ms-excel:");
    else if (powerpoint.contains(suffix))
        scheme = QStringLiteral("ms-powerpoint:");
    else if (project.contains(suffix))
        scheme = QStringLiteral("ms-project:");
    else
        return QString();

    return scheme + QStringLiteral("ofe|u|") + direct;
}

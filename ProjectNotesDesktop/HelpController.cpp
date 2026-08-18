// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "HelpController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QVariantMap>

// The docs/ tree is bundled under this prefix by qt_add_resources (CMakeLists.txt).
// Two spellings of the same location: ":/…" for QFile reads, "qrc:/…" for URLs
// embedded in Markdown that QML resolves as image sources.
static const QString kHelpFileRoot = QStringLiteral(":/qt/qml/ProjectNotesDesktop/help");
static const QString kHelpUrlRoot  = QStringLiteral("qrc:/qt/qml/ProjectNotesDesktop/help");

HelpController::HelpController(QObject* parent)
    : QObject(parent)
    , m_helpRoot(kHelpUrlRoot)
{
}

HelpController* HelpController::create(QQmlEngine*, QJSEngine*)
{
    return new HelpController();
}

QString HelpController::resourcePath(const QString& canonical)
{
    return kHelpFileRoot + "/" + canonical;
}

QString HelpController::readResource(const QString& canonical)
{
    QFile f(resourcePath(canonical));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll());
}

QString HelpController::canonicalize(const QString& raw, const QString& baseDir)
{
    QString s = raw.trimmed();

    // Angle-bracketed targets, as mkdocs allows for paths with spaces.
    if (s.startsWith('<') && s.endsWith('>'))
        s = s.mid(1, s.length() - 2).trimmed();

    // Drop any #anchor or ?query fragment.
    const int hash = s.indexOf('#');
    if (hash >= 0) s = s.left(hash);
    const int query = s.indexOf('?');
    if (query >= 0) s = s.left(query);

    s = QString::fromUtf8(QByteArray::fromPercentEncoding(s.trimmed().toUtf8()));
    if (s.isEmpty())
        return QString();

    QString combined;
    if (s.startsWith('/'))
        combined = s.mid(1);
    else if (baseDir.isEmpty())
        combined = s;
    else
        combined = baseDir + "/" + s;

    combined = QDir::cleanPath(combined);

    // cleanPath keeps leading ".." when a path escapes its base; treat the help
    // root as the ceiling and drop them.
    while (combined.startsWith("../"))
        combined = combined.mid(3);
    if (combined.startsWith("./"))
        combined = combined.mid(2);

    return combined;
}

const QString& HelpController::rawMarkdown(const QString& canonical) const
{
    auto it = m_cache.find(canonical);
    if (it == m_cache.end())
        it = m_cache.insert(canonical, readResource(canonical));
    return it.value();
}

QString HelpController::tocJson() const
{
    return readResource(QStringLiteral("help_toc.json"));
}

QString HelpController::topicTitle(const QString& relPath) const
{
    const QString canonical = canonicalize(relPath, QString());
    const QString md = rawMarkdown(canonical);

    // First ATX H1 ("# Title") wins.
    static const QRegularExpression reH1(QStringLiteral("(?m)^\\s*#\\s+(.+?)\\s*$"));
    const QRegularExpressionMatch m = reH1.match(md);
    if (m.hasMatch())
        return m.captured(1).trimmed();

    // Fall back to a prettified file name.
    QString base = QFileInfo(canonical).completeBaseName();
    base.replace('_', ' ');
    return base;
}

QString HelpController::topicMarkdown(const QString& relPath) const
{
    const QString canonical = canonicalize(relPath, QString());
    const QString md = rawMarkdown(canonical);
    if (md.isEmpty())
        return QStringLiteral("# Not Found\n\nThe help topic `%1` could not be loaded.").arg(relPath.toHtmlEscaped());

    const QString baseDir = canonical.contains('/') ? canonical.section('/', 0, -2) : QString();

    // Rewrite a relative image target into an absolute qrc: URL; leave external
    // or already-absolute targets untouched.
    auto rewriteUrl = [&](const QString& url) -> QString {
        const QString u = url.trimmed();
        if (u.startsWith("http://") || u.startsWith("https://") || u.startsWith("qrc:")
            || u.startsWith(":/") || u.startsWith("data:") || u.startsWith("mailto:") || u.startsWith("//"))
            return url;
        const QString canon = canonicalize(u, baseDir);
        if (canon.isEmpty())
            return url;
        return kHelpUrlRoot + "/" + canon;
    };

    QString out = md;

    // Markdown images: ![alt](target) where target may be <bracketed> and may
    // carry a "title". We keep alt + rewritten URL and drop any title (rare here).
    static const QRegularExpression reImg(QStringLiteral("!\\[([^\\]]*)\\]\\(([^)]*)\\)"));
    QString rebuilt;
    rebuilt.reserve(out.size() + 64);
    int last = 0;
    for (QRegularExpressionMatchIterator it = reImg.globalMatch(out); it.hasNext(); ) {
        const QRegularExpressionMatch m = it.next();
        rebuilt += out.mid(last, m.capturedStart() - last);

        const QString alt = m.captured(1);
        QString inside = m.captured(2).trimmed();
        QString urlPart;
        if (inside.startsWith('<')) {
            const int gt = inside.indexOf('>');
            urlPart = (gt >= 0) ? inside.mid(1, gt - 1) : inside.mid(1);
        } else {
            const int sp = inside.indexOf(QRegularExpression(QStringLiteral("\\s")));
            urlPart = (sp >= 0) ? inside.left(sp) : inside;
        }

        rebuilt += "![" + alt + "](" + rewriteUrl(urlPart) + ")";
        last = m.capturedEnd();
    }
    rebuilt += out.mid(last);
    out = rebuilt;

    // HTML <img src="..."> fallback.
    static const QRegularExpression reHtmlImg(QStringLiteral("(<img[^>]*\\bsrc\\s*=\\s*\")([^\"]+)(\")"),
                                              QRegularExpression::CaseInsensitiveOption);
    QString rebuilt2;
    rebuilt2.reserve(out.size());
    last = 0;
    for (QRegularExpressionMatchIterator it = reHtmlImg.globalMatch(out); it.hasNext(); ) {
        const QRegularExpressionMatch m = it.next();
        rebuilt2 += out.mid(last, m.capturedStart() - last);
        rebuilt2 += m.captured(1) + rewriteUrl(m.captured(2)) + m.captured(3);
        last = m.capturedEnd();
    }
    rebuilt2 += out.mid(last);

    return rebuilt2;
}

QString HelpController::resolveLink(const QString& fromPath, const QString& href) const
{
    QString h = href.trimmed();
    if (h.isEmpty())
        return QString();

    if (h.startsWith('<') && h.endsWith('>'))
        h = h.mid(1, h.length() - 2).trimmed();

    // External / non-topic links are handled by the caller (open in a browser).
    if (h.startsWith("http://") || h.startsWith("https://") || h.startsWith("mailto:")
        || h.startsWith("qrc:") || h.startsWith("//"))
        return QString();

    // Pure in-page anchor: stay on the current topic.
    if (h.startsWith('#'))
        return canonicalize(fromPath, QString());

    const QString fromCanonical = canonicalize(fromPath, QString());
    const QString baseDir = fromCanonical.contains('/') ? fromCanonical.section('/', 0, -2) : QString();
    const QString canon = canonicalize(h, baseDir);

    // Only Markdown topics that actually exist are navigable in-app.
    if (!canon.endsWith(".md", Qt::CaseInsensitive))
        return QString();
    if (!QFile::exists(resourcePath(canon)))
        return QString();

    return canon;
}

QVariantList HelpController::search(const QString& query) const
{
    QVariantList results;
    const QString needle = query.trimmed().toLower();
    if (needle.isEmpty())
        return results;

    // Walk the TOC to get the curated topic set (title + path).
    const QJsonDocument doc = QJsonDocument::fromJson(tocJson().toUtf8());
    const QJsonArray sections = doc.object().value("sections").toArray();

    for (const QJsonValue& sv : sections) {
        const QJsonArray items = sv.toObject().value("items").toArray();
        for (const QJsonValue& iv : items) {
            const QJsonObject item = iv.toObject();
            const QString path = item.value("path").toString();
            const QString title = item.value("title").toString();
            const QString body = rawMarkdown(canonicalize(path, QString()));

            const bool titleHit = title.toLower().contains(needle);
            const int bodyIdx = body.toLower().indexOf(needle);
            if (!titleHit && bodyIdx < 0)
                continue;

            // Build a short context snippet around the first body occurrence.
            QString snippet;
            if (bodyIdx >= 0) {
                const int start = qMax(0, bodyIdx - 40);
                snippet = body.mid(start, 140);
                snippet.replace(QRegularExpression(QStringLiteral("[#>*`\\r\\n]+")), " ");
                snippet = snippet.simplified();
                if (start > 0) snippet.prepend(QStringLiteral("… "));
                snippet.append(QStringLiteral(" …"));
            }

            QVariantMap row;
            row["title"] = title;
            row["path"] = path;
            row["snippet"] = snippet;
            row["rank"] = titleHit ? 0 : 1;
            results.append(row);
        }
    }

    // Title matches first, then by title alphabetically.
    std::sort(results.begin(), results.end(), [](const QVariant& a, const QVariant& b) {
        const QVariantMap ma = a.toMap();
        const QVariantMap mb = b.toMap();
        if (ma["rank"].toInt() != mb["rank"].toInt())
            return ma["rank"].toInt() < mb["rank"].toInt();
        return ma["title"].toString().localeAwareCompare(mb["title"].toString()) < 0;
    });

    return results;
}

void HelpController::applySpacing(QQuickTextDocument* document) const
{
    if (!document)
        return;
    QTextDocument* doc = document->textDocument();
    if (!doc)
        return;

    // Qt's Markdown importer sets tight, near-uniform block margins that read
    // as a dense slab of text at UI font sizes. Give headings real breathing
    // room above them, spread paragraphs apart, and keep list items snug
    // against their own siblings while still separating from what surrounds
    // the list. Line height gets a little extra too, since the default is
    // cramped for on-screen reading.
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        const int headingLevel = fmt.headingLevel();
        qreal top;
        qreal bottom;
        if (headingLevel > 0) {
            top = headingLevel <= 2 ? 22.0 : (headingLevel <= 4 ? 16.0 : 12.0);
            bottom = 6.0;
        } else if (block.textList()) {
            top = 1.0;
            bottom = 5.0;
        } else {
            top = 4.0;
            bottom = 14.0;
        }
        fmt.setTopMargin(top);
        fmt.setBottomMargin(bottom);
        fmt.setLineHeight(138.0, QTextBlockFormat::ProportionalHeight);
        cursor.setPosition(block.position());
        cursor.setBlockFormat(fmt);
    }
    cursor.endEditBlock();
}

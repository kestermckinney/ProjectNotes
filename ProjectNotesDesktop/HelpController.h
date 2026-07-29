// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef HELPCONTROLLER_H
#define HELPCONTROLLER_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantList>

class QQmlEngine;
class QJSEngine;

// HelpController — QML singleton backing the in-app User Guide (HelpPage.qml).
//
// The help content is the same Markdown that builds the mkdocs website: the
// docs/ tree is bundled into the QML module's resources under
// qrc:/qt/qml/ProjectNotesDesktop/help/ (see CMakeLists.txt). This controller
// owns the table of contents (help_toc.json), serves a topic's Markdown with its
// image links rewritten to absolute qrc: URLs (QML Text has no baseUrl, so
// relative ../img/... paths would not otherwise resolve), resolves in-topic
// links to canonical topic paths, and provides a simple full-text search.
//
// All topic paths used across the API are canonical: forward-slashed, relative
// to the help root, with any surrounding < > and ./ or ../ segments resolved
// (e.g. "InterfaceOverview/ProjectPage.md").
class HelpController : public QObject
{
    Q_OBJECT

public:
    explicit HelpController(QObject* parent = nullptr);

    static HelpController* create(QQmlEngine* engine, QJSEngine* scriptEngine);

    // The table of contents as a JSON string: { "sections": [ { title, items:
    // [ { title, path } ] } ] }. Read verbatim from the bundled help_toc.json.
    Q_INVOKABLE QString tocJson() const;

    // Markdown for a topic, with relative image URLs rewritten to absolute qrc:
    // URLs so QML's Text.MarkdownText can display them. Returns a short "not
    // found" Markdown message if the topic does not exist.
    Q_INVOKABLE QString topicMarkdown(const QString& relPath) const;

    // The display title for a topic (its first Markdown H1, else the file name).
    Q_INVOKABLE QString topicTitle(const QString& relPath) const;

    // Resolve a link target found inside topic |fromPath| to a canonical topic
    // path, or return "" when it is not a known in-app topic (e.g. an http(s)
    // link, which the caller should open externally).
    Q_INVOKABLE QString resolveLink(const QString& fromPath, const QString& href) const;

    // Case-insensitive search over topic titles and body text. Returns a list of
    // { title, path, snippet } maps, title matches ranked first.
    Q_INVOKABLE QVariantList search(const QString& query) const;

private:
    // qrc file path ("...:/help/<canonical>") for a canonical topic path.
    static QString resourcePath(const QString& canonical);
    // Canonicalize a raw path/link: strip < >, a leading help root, resolve
    // . and .. segments relative to |baseDir| (the directory of the referring
    // topic, "" for none). Query/anchor fragments are dropped.
    static QString canonicalize(const QString& raw, const QString& baseDir);
    // Read a bundled help resource as UTF-8 text ("" if missing).
    static QString readResource(const QString& canonical);

    // Lazily read and cache a topic's raw Markdown (keyed by canonical path).
    const QString& rawMarkdown(const QString& canonical) const;

    QString m_helpRoot;                       // qrc prefix, no trailing slash
    mutable QHash<QString, QString> m_cache;  // canonical path -> raw markdown
};

#endif // HELPCONTROLLER_H

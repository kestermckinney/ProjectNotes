// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <QTextDocument>
#include <QtTest/QtTest>

#include "ProjectNotesEmailRendering/RendererResourcePolicy.h"

class RichTextPolicyTest final : public QObject
{
    Q_OBJECT

private slots:
    void retainsBasicProseFormattingWithoutExecution();
    void preservesExternalResourceReferences();
    void blocksRendererSubresources();
};

void RichTextPolicyTest::retainsBasicProseFormattingWithoutExecution()
{
    QTextDocument document;
    document.setHtml(QStringLiteral("<p>Intro <b>bold</b> and <i>italic</i>.</p>"
                                    "<script>window.nativeTemplateProbe = true;</script>"));

    const QString normalized = document.toHtml();
    QVERIFY(document.toPlainText().contains(QStringLiteral("Intro bold and italic.")));
    QVERIFY(normalized.contains(QStringLiteral("font-weight")));
    QVERIFY(normalized.contains(QStringLiteral("font-style")));
    QVERIFY(!normalized.contains(QStringLiteral("nativeTemplateProbe")));
    QVERIFY(!normalized.contains(QStringLiteral("<script"), Qt::CaseInsensitive));
}

void RichTextPolicyTest::preservesExternalResourceReferences()
{
    QTextDocument document;
    document.setHtml(QStringLiteral("<p><img src=\"https://example.invalid/pixel.png\"></p>"));

    // Parsing itself must not be mistaken for a resource policy: the canonical
    // Qt output still contains this network URL, so this round trip alone is
    // insufficient for imported template prose under G5.
    QVERIFY(document.toHtml().contains(QStringLiteral("https://example.invalid/pixel.png")));
}

void RichTextPolicyTest::blocksRendererSubresources()
{
    using PN::Comm::rendererResourceRequestAllowed;
    QVERIFY(rendererResourceRequestAllowed(QUrl(QStringLiteral("about:blank")), true));
    QVERIFY(rendererResourceRequestAllowed(QUrl(QStringLiteral("data:text/html,body")), true));
    QVERIFY(!rendererResourceRequestAllowed(QUrl(QStringLiteral("https://example.invalid/pixel.png")), false));
    QVERIFY(!rendererResourceRequestAllowed(QUrl(QStringLiteral("file:///tmp/private.txt")), false));
    QVERIFY(!rendererResourceRequestAllowed(QUrl(QStringLiteral("qrc:/private-resource")), false));
    QVERIFY(!rendererResourceRequestAllowed(QUrl(QStringLiteral("data:image/png;base64,AA==")), false));
    QVERIFY(!rendererResourceRequestAllowed(QUrl(QStringLiteral("https://example.invalid/")), true));
}

QTEST_MAIN(RichTextPolicyTest)
#include "tst_richtextpolicy.moc"

// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "officedeeplink.h"

#include <QtTest>

class TstOfficeDeepLink : public QObject
{
    Q_OBJECT

private slots:
    void directDocumentUrls_data();
    void directDocumentUrls();
    void queryAndFragmentAreDropped();
    void viewerPagesAndSharingLinksReturnEmpty();
    void nonOfficeUrlsReturnEmpty();
    void localPathsReturnEmpty();
    void legacyRowsAreUnwrapped();
};

void TstOfficeDeepLink::directDocumentUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expectedScheme");

    const QString base = QStringLiteral("https://contoso.sharepoint.com/sites/ProjectX/Shared%20Documents/General/");
    QTest::newRow("docx") << base + "Report.docx" << "ms-word:";
    QTest::newRow("doc")  << base + "Report.doc"  << "ms-word:";
    QTest::newRow("xlsx") << base + "Budget.xlsx" << "ms-excel:";
    QTest::newRow("xlsm") << base + "Budget.xlsm" << "ms-excel:";
    QTest::newRow("pptx") << base + "Deck.pptx"   << "ms-powerpoint:";
    QTest::newRow("ppt")  << base + "Deck.ppt"    << "ms-powerpoint:";
    QTest::newRow("mpp")  << base + "Plan.mpp"    << "ms-project:";
    QTest::newRow("unencoded spaces")
        << "https://contoso.sharepoint.com/sites/ProjectX/Shared Documents/Status Report.XLSX" << "ms-excel:";
}

void TstOfficeDeepLink::directDocumentUrls()
{
    QFETCH(QString, url);
    QFETCH(QString, expectedScheme);
    QCOMPARE(officeDeepLinkFor(url), expectedScheme + "ofe|u|" + url);
}

void TstOfficeDeepLink::queryAndFragmentAreDropped()
{
    QCOMPARE(officeDeepLinkFor("https://contoso.sharepoint.com/sites/X/Shared%20Documents/Budget.xlsx?web=1#top"),
             QStringLiteral("ms-excel:ofe|u|https://contoso.sharepoint.com/sites/X/Shared%20Documents/Budget.xlsx"));
}

void TstOfficeDeepLink::viewerPagesAndSharingLinksReturnEmpty()
{
    // Both forms seen in real File Finder / pasted rows. Neither is the
    // document itself, so Office reports "doesn't recognize the command".
    QVERIFY(officeDeepLinkFor(
        "https://contoso.sharepoint.com/sites/X/_layouts/15/Doc.aspx"
        "?sourcedoc=%7BGUID%7D&file=Estimate%20Template.xlsx").isEmpty());
    QVERIFY(officeDeepLinkFor(
        "https://contoso.sharepoint.com/:x:/r/Sales/_layouts/15/Doc.aspx"
        "?sourcedoc=%7BGUID%7D&file=Estimate.xlsx&action=default&mobileredirect=true").isEmpty());
    QVERIFY(officeDeepLinkFor(
        "https://contoso.sharepoint.com/:f:/g/Sales/IgAUe2pEQ5MOR49ZI7oU1JYh?e=mf5ybL").isEmpty());
}

void TstOfficeDeepLink::nonOfficeUrlsReturnEmpty()
{
    QVERIFY(officeDeepLinkFor("https://contoso.sharepoint.com/sites/X/Manual.pdf").isEmpty());
    QVERIFY(officeDeepLinkFor("https://contoso.sharepoint.com/sites/X/Notes.odt").isEmpty());
    QVERIFY(officeDeepLinkFor("https://example.com/some/page").isEmpty());
}

void TstOfficeDeepLink::localPathsReturnEmpty()
{
    QVERIFY(officeDeepLinkFor("C:/Projects/ProjectX/Report.docx").isEmpty());
    QVERIFY(officeDeepLinkFor("/home/user/Projects/ProjectX/Report.docx").isEmpty());
    QVERIFY(officeDeepLinkFor("").isEmpty());
}

void TstOfficeDeepLink::legacyRowsAreUnwrapped()
{
    const QString viewer = QStringLiteral(
        "https://contoso.sharepoint.com/:x:/r/Sales/_layouts/15/Doc.aspx?sourcedoc=%7BGUID%7D&file=Estimate.xlsx");
    QCOMPARE(unwrapOfficeDeepLink("ms-excel:ofe|u|" + viewer), viewer);
    QCOMPARE(unwrapOfficeDeepLink("MS-WORD:ofv|u|https://a.test/b.docx"), QStringLiteral("https://a.test/b.docx"));
    QCOMPARE(unwrapOfficeDeepLink("https://a.test/b.docx"), QStringLiteral("https://a.test/b.docx"));
    QCOMPARE(unwrapOfficeDeepLink("C:/local/b.docx"), QStringLiteral("C:/local/b.docx"));

    // A legacy row wrapping a viewer page still gets no deep link...
    QVERIFY(officeDeepLinkFor("ms-excel:ofe|u|" + viewer).isEmpty());
    // ...while one wrapping a direct URL is rebuilt identically.
    QCOMPARE(officeDeepLinkFor("ms-excel:ofe|u|https://a.test/Budget.xlsx"),
             QStringLiteral("ms-excel:ofe|u|https://a.test/Budget.xlsx"));
}

QTEST_APPLESS_MAIN(TstOfficeDeepLink)
#include "tst_officedeeplink.moc"

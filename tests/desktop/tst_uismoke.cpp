// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
//
// UiSmokeTest — loads the REAL Main.qml headless (offscreen) and drives the same
// navigation entry points the shell uses (selectSection / openProject / openNote
// / openItem / openPerson / openClient / openHelp / handleMenuAction). Any QML
// runtime warning emitted while a screen is built or shown fails the test, so it
// proves "the application does not error" across every page. It relies on the
// data the parity group seeds first (same process, same database).

#include <QtTest/QtTest>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QQuickWindow>
#include <QTextBlock>
#include <QTextDocument>
#include <QVariant>

#include "DesktopAppController.h"

class UiSmokeTest : public QObject
{
    Q_OBJECT

    QQmlApplicationEngine* engine = nullptr;
    QObject* root = nullptr;
    DesktopAppController* c = nullptr;
    QStringList m_warnings;

    // Invoke a QML document function on the root window with QVariant args.
    void nav(const char* fn,
             const QVariant& a1 = QVariant(),
             const QVariant& a2 = QVariant(),
             const QVariant& a3 = QVariant())
    {
        bool ok = false;
        if (!a3.isValid() && !a2.isValid() && !a1.isValid())
            ok = QMetaObject::invokeMethod(root, fn);
        else if (!a3.isValid() && !a2.isValid())
            ok = QMetaObject::invokeMethod(root, fn, Q_ARG(QVariant, a1));
        else if (!a3.isValid())
            ok = QMetaObject::invokeMethod(root, fn, Q_ARG(QVariant, a1), Q_ARG(QVariant, a2));
        else
            ok = QMetaObject::invokeMethod(root, fn,
                     Q_ARG(QVariant, a1), Q_ARG(QVariant, a2), Q_ARG(QVariant, a3));
        QVERIFY2(ok, qPrintable(QStringLiteral("invoke failed: %1").arg(fn)));
        QTest::qWait(60);   // let the pushed page build + bindings evaluate
    }

    void clearWarnings() { m_warnings.clear(); }
    void assertClean(const char* where)
    {
        if (!m_warnings.isEmpty())
            QFAIL(qPrintable(QStringLiteral("QML warning(s) during %1:\n  %2")
                             .arg(where, m_warnings.join("\n  "))));
    }

private slots:
    void initTestCase()
    {
        c = DesktopAppController::create(nullptr, nullptr);
        QVERIFY(c);
        QVERIFY(c->databaseOpen());   // opened + seeded by the parity group

        engine = new QQmlApplicationEngine(this);
        connect(engine, &QQmlApplicationEngine::warnings, this,
                [this](const QList<QQmlError>& ws) {
                    for (const QQmlError& e : ws) m_warnings << e.toString();
                });

        engine->load(QUrl("qrc:/qt/qml/ProjectNotesDesktop/qml/Main.qml"));
        QTRY_VERIFY_WITH_TIMEOUT(!engine->rootObjects().isEmpty(), 5000);
        root = engine->rootObjects().first();
        QVERIFY(root);
        QTest::qWait(200);   // let Component.onCompleted (DB open + folder load) settle
    }

    // Loading the shell must be warning-free.
    void test_00_loadClean()
    {
        assertClean("initial load of Main.qml");
    }

    // Visit every primary section of the shell.
    void test_01_sections()
    {
        const QStringList sections = {
            "projects", "items", "people", "clients", "search", "settings", "help", "projects"
        };
        for (const QString& s : sections) {
            clearWarnings();
            nav("selectSection", s);
            assertClean(qPrintable(QStringLiteral("selectSection('%1')").arg(s)));
        }
    }

    void test_02_projectDetailAndChildren()
    {
        const QString pid = firstId(c->projectsListModel());
        QVERIFY2(!pid.isEmpty(), "no seeded project to open");

        clearWarnings();
        nav("selectSection", "projects");
        nav("openProject", pid);
        assertClean("openProject");

        // Open a note under the project.
        c->setProjectFilter(pid);
        const QString noteId = c->projectNoteIdAtRow(0);
        if (!noteId.isEmpty()) {
            clearWarnings();
            nav("openNote", 0, noteId, pid);
            assertClean("openNote");
            nav("goBack");
        }
    }

    void test_02b_settingsCategories()
    {
        clearWarnings();
        nav("selectSection", "settings");
        QQuickWindow* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        QQuickItem* tabList = findVisualChild(window->contentItem(), QStringLiteral("settingsTabList"));
        QQuickItem* pages = findVisualChild(window->contentItem(), QStringLiteral("settingsPages"));
        QVERIFY(tabList);
        QVERIFY(pages);
        QCOMPARE(tabList->property("count").toInt(), 6);
        QCOMPARE(pages->property("count").toInt(), 6);
        assertClean("settings category tabs");
    }

    void test_03_itemDetail()
    {
        clearWarnings();
        nav("selectSection", "items");
        c->refreshAllItems();
        const QString itemId = c->allItemIdAtRow(0);
        if (!itemId.isEmpty()) {
            nav("openItem", itemId);
            assertClean("openItem");
        }
    }

    void test_04_personDetail()
    {
        clearWarnings();
        nav("selectSection", "people");
        const QString id = firstId(c->peopleModel());
        if (!id.isEmpty()) {
            nav("openPerson", c->peopleRowForId(id), id);
            assertClean("openPerson");
        }
    }

    void test_05_clientDetail()
    {
        clearWarnings();
        nav("selectSection", "clients");
        const QString id = firstId(c->clientsModel());
        if (!id.isEmpty()) {
            nav("openClient", c->clientRowForId(id), id);
            assertClean("openClient");
        }
    }

    void test_06_help()
    {
        clearWarnings();
        nav("openHelp", "index.md");
        assertClean("openHelp(index.md)");
        nav("openHelp", "InterfaceOverview/ProjectListPage.md");
        assertClean("openHelp(topic)");
    }

    // Regression: HelpPage's Markdown viewer must actually expose a
    // textDocument — that property exists on TextEdit/TextArea but NOT on
    // plain Text, so a prior version of this page (a Text item) silently fed
    // HelpController::applySpacing() an undefined document and never widened
    // heading margins at all. Opens a topic with "##" section headers and
    // checks a heading block's top margin was actually widened.
    void test_06b_helpHeadingSpacing()
    {
        clearWarnings();
        nav("openHelp", "InterfaceOverview/FindAndReplace.md");
        assertClean("openHelp(FindAndReplace.md)");

        QQuickTextDocument* qqDoc = nullptr;
        for (QQuickItem* item : root->findChildren<QQuickItem*>()) {
            if (qstrcmp(item->metaObject()->className(), "QQuickTextEdit") != 0)
                continue;
            const QVariant v = item->property("textDocument");
            if (v.isValid() && (qqDoc = v.value<QQuickTextDocument*>()))
                break;
        }
        QVERIFY2(qqDoc, "help content TextEdit (docText) not found, or has no textDocument");
        QTextDocument* doc = qqDoc->textDocument();
        QVERIFY(doc);

        bool sawWidenedHeading = false;
        for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
            if (b.blockFormat().headingLevel() > 0 && b.blockFormat().topMargin() >= 12.0)
                sawWidenedHeading = true;
        }
        QVERIFY2(sawWidenedHeading, "no heading block had a widened top margin — applySpacing() did not run");
    }

    void test_07_menuActions()
    {
        for (const QString& action : { QStringLiteral("filter"),
                                       QStringLiteral("logs"),
                                       QStringLiteral("preferences"),
                                       QStringLiteral("help"),
                                       QStringLiteral("about") }) {
            clearWarnings();
            nav("selectSection", "projects");
            nav("handleMenuAction", action);
            assertClean(qPrintable(QStringLiteral("handleMenuAction('%1')").arg(action)));
            if (action == QStringLiteral("about")) {
                QObject* about = root->findChild<QObject*>(QStringLiteral("aboutDialog"));
                QVERIFY(about);
                QVERIFY(about->property("visible").toBool());
                QVERIFY(QMetaObject::invokeMethod(about, "close"));
            }
        }
    }

    void cleanupTestCase()
    {
        delete engine;   // tear the window down before the app object goes away
        engine = nullptr;
    }

private:
    static QQuickItem* findVisualChild(QQuickItem* item, const QString& objectName)
    {
        if (!item)
            return nullptr;
        if (item->objectName() == objectName)
            return item;
        for (QQuickItem* child : item->childItems()) {
            if (QQuickItem* match = findVisualChild(child, objectName))
                return match;
        }
        return nullptr;
    }

    static QString firstId(QAbstractItemModel* m)
    {
        if (!m || m->rowCount() < 1) return {};
        return m->data(m->index(0, 0)).toString();
    }
};

int runUiSmokeTests(int argc, char** argv)
{
    UiSmokeTest t;
    return QTest::qExec(&t, argc, argv);
}

#include "tst_uismoke.moc"

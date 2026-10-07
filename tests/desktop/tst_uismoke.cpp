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
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickTextDocument>
#include <QQuickWindow>
#include <QTextBlock>
#include <QTextDocument>
#include <QVariant>

#include "DesktopAppController.h"
#include "ProjectNotesIntegrations/MeetingTemplateStore.h"
#include "ProjectNotesContacts/ContactSyncSettingsModel.h"
#include "databaseobjects.h"
#include <QQmlComponent>
#include <QTemporaryDir>
#include <QSqlQuery>

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

    void test_00a_projectEmailAndLocationMenusAndReviewLoadClean()
    {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(root);
        QVERIFY(window);
        // Main.qml starts hidden; the shipping main.cpp calls show(). Map the
        // offscreen window too so Overlay has its real size for visual checks.
        window->show();
        QTest::qWait(100);
        const QString projectId = c->projectIdAtRow(0);
        QVERIFY(!projectId.isEmpty());
        QQmlComponent menuComponent(engine, QUrl("qrc:/qt/qml/ProjectNotesDesktop/qml/RecordContextMenu.qml"));
        QScopedPointer<QObject> menu(menuComponent.create());
        QVERIFY2(menu, qPrintable(menuComponent.errorString()));
        menu->setProperty("recordTable", "projects");
        menu->setProperty("recordId", projectId);
        QVERIFY(menu->property("canProjectEmail").toBool());
        QVERIFY(!menu->property("canShareLocation").toBool());
        menu->setProperty("recordTable", "people");
        menu->setProperty("emailProjectId", "");
        menu->setProperty("personId", "");
        QVERIFY(menu->property("canPersonEmail").toBool());
        QVERIFY(!menu->property("canProjectEmail").toBool());
        menu->setProperty("recordTable", "project_locations");
        menu->setProperty("emailProjectId", projectId);
        menu->setProperty("locationPath", "https://example.test/file.pdf");
        QVERIFY(menu->property("canShareLocation").toBool());
        QVERIFY(menu->property("canShareLink").toBool());
        menu->setProperty("locationPath", "/tmp/file.pdf");
        QVERIFY(!menu->property("canShareLink").toBool());
        menu->setProperty("recordTable", "meeting_attendees");
        menu->setProperty("personId", "person");
        QVERIFY(menu->property("canProjectEmail").toBool());
        QVERIFY(!menu->property("canShareLocation").toBool());
        // Use a menu-owned dialog in the real scene so its Overlay parent and
        // popup layout match the shipping application.
        QObject *dialog = nullptr;
        for (QObject *candidate : root->findChildren<QObject *>("projectEmailReviewDialog")) {
            auto *parentItem = qvariant_cast<QQuickItem *>(candidate->property("parent"));
            if (parentItem && parentItem->window() == window) { dialog = candidate; break; }
        }
        QVERIFY(dialog);
        QVERIFY(QMetaObject::invokeMethod(dialog, "openProject", Q_ARG(QVariant, projectId), Q_ARG(QVariant, QString())));
        QTRY_VERIFY_WITH_TIMEOUT(dialog->property("ready").toBool(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1000);
        QVERIFY(dialog->property("width").toDouble() > 0);
        QVERIFY(dialog->property("height").toDouble() > 0);
        QTest::qWait(300);
        QVERIFY(!dialog->findChild<QQuickItem *>("projectEmailSubject"));
        QVERIFY(!dialog->findChild<QQuickItem *>("projectEmailBody"));
        auto *draftButton = dialog->findChild<QQuickItem *>("projectEmailDraftButton");
        auto *audience = dialog->findChild<QQuickItem *>("projectEmailAudience");
        QVERIFY(draftButton); QVERIFY(audience);
        QVERIFY(audience->isVisible());
        QVERIFY(!audience->property("fixedAudience").toBool());
        QVERIFY(!audience->property("audienceController").isNull());
        const qreal audienceRight = audience->mapToScene(QPointF(audience->width(), 0)).x();
        const qreal buttonRight = draftButton->mapToScene(QPointF(draftButton->width(), 0)).x();
        QVERIFY2(qAbs(audienceRight - buttonRight) < 1.0, qPrintable(QString("audience=%1 button=%2").arg(audienceRight).arg(buttonRight)));
        const QImage screenshot = window->grabWindow();
        if (!screenshot.isNull()) screenshot.save(QDir::temp().filePath("projectnotes-project-email-review.png"));
        assertClean("Project Email and location sharing menus/review");
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        const auto people = c->peopleList();
        QVERIFY(!people.isEmpty());
        QVERIFY(QMetaObject::invokeMethod(dialog, "openPerson",
            Q_ARG(QVariant, people.first().toMap().value("id"))));
        QTRY_VERIFY_WITH_TIMEOUT(dialog->property("ready").toBool(), 5000);
        QVERIFY(dialog->property("plainPerson").toBool());
        QVERIFY(!audience->isVisible());
        QVERIFY(!dialog->findChild<QQuickItem *>("projectEmailSubject"));
        QVERIFY(!dialog->findChild<QQuickItem *>("projectEmailBody"));
        assertClean("plain person email review");
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        window->hide();
    }

    void test_00b_emailTemplatesCanBeFirstSettingsCategorySelected()
    {
        nav("selectSection", "settings");
        QQuickWindow* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        QQuickItem* settings = findVisualChild(window->contentItem(), QStringLiteral("settingsPage"));
        QQuickItem* pages = findVisualChild(window->contentItem(), QStringLiteral("settingsPages"));
        QVERIFY(settings);
        QVERIFY(pages);
        QVariant selected;
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 3)));
        QVERIFY(selected.toBool());
        QTRY_COMPARE_WITH_TIMEOUT(pages->property("currentIndex").toInt(), 3, 1000);
        assertClean("Email Templates selected first");
    }

    void test_00c_meetingTemplatesTabEditsClean()
    {
        nav("selectSection", "settings");
        QQuickWindow* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        QQuickItem* settings = findVisualChild(window->contentItem(), QStringLiteral("settingsPage"));
        QQuickItem* pages = findVisualChild(window->contentItem(), QStringLiteral("settingsPages"));
        QVERIFY(settings);
        QVERIFY(pages);
        QVariant selected;
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 4)));
        QVERIFY(selected.toBool());
        QTRY_COMPARE_WITH_TIMEOUT(pages->property("currentIndex").toInt(), 4, 1000);
        assertClean("Meeting Templates selected");

        // Showing the editor for a new, blank meeting type must bind cleanly,
        // and leaving the tab discards it rather than blocking navigation.
        QObject* model = c->meetingTemplateEditorModel();
        QVERIFY(model);
        QVERIFY(QMetaObject::invokeMethod(model, "beginNew"));
        QTRY_VERIFY_WITH_TIMEOUT(model->property("hasDraft").toBool(), 1000);
        assertClean("new meeting type editor");
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 0)));
        QVERIFY(selected.toBool());
        QVERIFY(!model->property("hasDraft").toBool());
        assertClean("blank meeting type discarded");

        // A named draft with no subject is kept but blocks leaving the tab
        // behind the themed validation dialog.
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 4)));
        QVERIFY(QMetaObject::invokeMethod(model, "beginNew"));
        model->setProperty("draftName", QStringLiteral("Smoke meeting"));
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 0)));
        QVERIFY(!selected.toBool());
        QObject* dialog = root->findChild<QObject*>(QStringLiteral("meetingValidationDialog"));
        QVERIFY(dialog);
        QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1000);
        QVERIFY(!dialog->property("message").toString().isEmpty());
        assertClean("meeting validation dialog");
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QVERIFY(QMetaObject::invokeMethod(model, "deleteSelected"));
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 0)));
        QVERIFY(selected.toBool());
        assertClean("meeting type deleted");
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
        const QString pid = projectWithNote();
        QVERIFY2(!pid.isEmpty(), "no seeded project to open");

        clearWarnings();
        nav("selectSection", "projects");
        nav("openProject", pid);
        assertClean("openProject");
        {
            auto *window = qobject_cast<QQuickWindow*>(root);
            auto *first = findVisualChild(window->contentItem(), "Project NumberInput");
            QVERIFY(first);
            const bool showInternal = c->showInternalItems();
            c->setShowInternalItems(true);
            const QStringList order = {"Project NumberInput", "Project NameInput", "StatusInput",
                                       "ClientInput", "Primary ContactInput", "Status DateInput",
                                       "Invoice DateInput", "Invoicing PeriodInput",
                                       "Status Report PeriodInput", "BudgetInput", "Actual (ACWP)Input",
                                       "BCWPInput", "BCWSInput", "BACInput"};
            auto focusedWithin = [window](const QString& name) {
                for (auto *item = window->activeFocusItem(); item; item = item->parentItem())
                    if (item->objectName() == name) return true;
                return false;
            };
            first->forceActiveFocus();
            for (const QString& name : order) {
                QVERIFY2(focusedWithin(name), qPrintable("Tab focus missed " + name));
                QTest::keyClick(window, Qt::Key_Tab);
            }
            auto *last = findVisualChild(window->contentItem(), "BACInput");
            QVERIFY(last);
            last->forceActiveFocus();
            for (auto it = order.crbegin(); it != order.crend(); ++it) {
                QVERIFY2(focusedWithin(*it), qPrintable("Shift+Tab focus missed " + *it));
                QTest::keyClick(window, Qt::Key_Backtab);
            }
            const QSize originalSize = window->size();
            window->resize(850, originalSize.height());
            QTest::qWait(50);
            first->forceActiveFocus();
            for (const QString& name : order) {
                QVERIFY2(focusedWithin(name), qPrintable("Narrow layout Tab focus missed " + name));
                QTest::keyClick(window, Qt::Key_Tab);
            }
            window->resize(originalSize);
            c->setShowInternalItems(showInternal);
        }

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
        QCOMPARE(tabList->property("count").toInt(), 10);
        QCOMPARE(pages->property("count").toInt(), 10);
        assertClean("settings category tabs");
    }

    void test_02bb_preferencesSearchKeyboardSelection()
    {
        const QString originalId = c->managingCompanyId();
        const QString firstName = QStringLiteral("KeyboardChoice Alpha");
        const QString secondName = QStringLiteral("KeyboardChoice Beta");
        QStringList ids;
        for (const QString& name : {firstName, secondName}) {
            const int row = c->addClient();
            QVERIFY(row >= 0);
            QVERIFY2(c->saveClient(row, name), qPrintable(c->lastSaveError()));
            for (const QVariant& entry : c->clientList()) {
                const QVariantMap client = entry.toMap();
                if (client.value(QStringLiteral("name")).toString() == name) {
                    ids.append(client.value(QStringLiteral("id")).toString());
                    break;
                }
            }
        }
        QCOMPARE(ids.size(), 2);

        clearWarnings();
        nav("selectSection", "settings");
        auto *window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        QQuickItem* settings = findVisualChild(window->contentItem(), QStringLiteral("settingsPage"));
        QVERIFY(settings);
        QVERIFY(QMetaObject::invokeMethod(settings, "refreshContactOptions"));
        QVariant selected;
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 7)));
        QVERIFY(selected.toBool());
        QQuickItem* combo = findVisualChild(window->contentItem(), QStringLiteral("Managing CompanyInput"));
        QVERIFY(combo);
        QQuickItem* editor = combo->property("contentItem").value<QQuickItem*>();
        QVERIFY(editor);
        auto typeText = [window](const QString& value) {
            for (QChar ch : value)
                QTest::keyClick(window, static_cast<Qt::Key>(ch.toUpper().unicode()),
                                ch.isUpper() ? Qt::ShiftModifier : Qt::NoModifier);
        };
        editor->forceActiveFocus();
        typeText(QStringLiteral("KeyboardChoice"));
        QTRY_COMPARE(combo->property("_filter").toString(), QStringLiteral("keyboardchoice"));

        QTest::keyClick(window, Qt::Key_Down);
        const int firstIndex = combo->property("_keyboardIndex").toInt();
        QVERIFY(firstIndex >= 0);
        QTest::keyClick(window, Qt::Key_Down);
        const int secondIndex = combo->property("_keyboardIndex").toInt();
        QVERIFY(secondIndex > firstIndex);
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(combo->property("_keyboardIndex").toInt(), secondIndex);
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(combo->property("_keyboardIndex").toInt(), firstIndex);
        QCOMPARE(c->managingCompanyId(), originalId);

        QTest::keyClick(window, Qt::Key_Backspace);
        QCOMPARE(combo->property("_keyboardIndex").toInt(), -1);
        QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
        typeText(QStringLiteral("NoMatchingClientName"));
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(combo->property("_keyboardIndex").toInt(), -1);
        QTest::keyClick(window, Qt::Key_Return);
        QCOMPARE(c->managingCompanyId(), originalId);

        QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
        typeText(QStringLiteral("KeyboardChoice"));
        QTest::keyClick(window, Qt::Key_Down);
        QTest::keyClick(window, Qt::Key_Down);
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_COMPARE(c->managingCompanyId(), ids.at(1));
        assertClean("preferences keyboard autocomplete");

        c->setManagingCompanyId(originalId);
        for (const QString& id : ids) {
            const int row = c->clientRowForId(id);
            QVERIFY(row >= 0);
            QVERIFY2(c->deleteClient(row), qPrintable(c->lastSaveError()));
        }
        QVERIFY(QMetaObject::invokeMethod(settings, "refreshContactOptions"));
    }

    void test_02c_nativeContactsReviewPane()
    {
        using namespace PN::Contacts;
        class ReviewProvider final : public ContactProvider {
        public:
            bool clientReview=false;
            void fetch(const QVariantMap &, const QJsonObject &, Completion done) override {
                Contact contact; contact.id=clientReview?"client-missing-name":"missing-name"; contact.book="book";
                if(clientReview){contact.company="Review Test Company - Boston";contact.fields["company"]=contact.company;}
                Snapshot snapshot; snapshot.account="review-test"; snapshot.contacts={contact};
                snapshot.books={QVariantMap{{"id","book"},{"name","Contacts"},{"writable",true}}};
                done(snapshot);
            }
            void create(const QVariantMap &,const Contact &,const QString &,Created done) override {done({},"Unexpected export");}
            void update(const QVariantMap &,const Contact &,const Contact &,Updated done) override {done({},"Unexpected update");}
            void cancel() override {}
        } provider;
        QTemporaryDir state;
        ContactSyncSettingsModel model(&global_DBObjects,nullptr,"ProjectNotesqmltest",state.path());
        model.setProviderForTesting(&provider);model.setDatabase(global_DBObjects.getDatabaseFile());
        model.setOption("startup",false);model.setOption("enabled",true);model.syncNow();
        QTRY_VERIFY(!model.busy());QCOMPARE(model.reviews().size(),1);
        clearWarnings();
        QQmlComponent component(engine,QUrl("qrc:/qt/qml/ProjectNotesDesktop/qml/ContactSyncSettingsPane.qml"));
        std::unique_ptr<QObject> pane(component.createWithInitialProperties({{"syncModel",QVariant::fromValue(static_cast<QObject*>(&model))}}));
        QVERIFY2(pane,qPrintable(component.errorString()));
        QVERIFY(!pane->findChild<QObject *>(QStringLiteral("contactSourceCombo")));
        for (auto *item : pane->findChildren<QQuickItem *>()) {
            QVERIFY(item->property("text").toString() != QStringLiteral("Sign in for contacts"));
            QVERIFY(item->property("text").toString() != QStringLiteral("Export destination"));
        }
        auto *paneItem = qobject_cast<QQuickItem *>(pane.get());
        auto *window = qobject_cast<QQuickWindow *>(root);
        QVERIFY(paneItem);
        QVERIFY(window);
        paneItem->setParentItem(window->contentItem());
        auto *reviewButton = pane->findChild<QObject *>(QStringLiteral("reviewContactConflictsButton"));
        auto *reviewDialog = pane->findChild<QObject *>(QStringLiteral("contactConflictReviewDialog"));
        auto *exportButton = pane->findChild<QObject *>(QStringLiteral("fullContactExportButton"));
        auto *exportWarning = pane->findChild<QObject *>(QStringLiteral("fullContactExportWarning"));
        auto *exportReview = pane->findChild<QObject *>(QStringLiteral("contactExportReviewDialog"));
        QVERIFY(reviewButton);
        QVERIFY(reviewDialog);
        QVERIFY(exportButton);
        QCOMPARE(exportButton->property("text").toString(),QStringLiteral("Full Contact Export"));
        QVERIFY(exportWarning);
        QVERIFY(exportReview);
        QVERIFY(QMetaObject::invokeMethod(exportButton,"clicked"));
        QTRY_VERIFY(exportWarning->property("visible").toBool());
        QVERIFY(exportWarning->property("message").toString().contains(QStringLiteral("update, skip, or create a new contact")));
        QVERIFY(QMetaObject::invokeMethod(exportWarning,"close"));
        auto *bridgeButton = pane->findChild<QObject *>(QStringLiteral("thunderbirdBridgeSetupButton"));
        auto *bridgeConfirm = pane->findChild<QObject *>(QStringLiteral("thunderbirdBridgeSetupConfirm"));
        auto *bridgeResult = pane->findChild<QObject *>(QStringLiteral("thunderbirdBridgeSetupResult"));
        QVERIFY(bridgeButton);QVERIFY(bridgeConfirm);QVERIFY(bridgeResult);
        QVERIFY(QMetaObject::invokeMethod(bridgeButton,"clicked"));
        QTRY_VERIFY(bridgeConfirm->property("visible").toBool());
        QVERIFY(bridgeConfirm->property("message").toString().contains("install the Project Notes Contacts add-on"));
        QVERIFY(QMetaObject::invokeMethod(bridgeConfirm,"close"));
        model.thunderbirdSetupFinished("Fixture installation steps",true);
        QTRY_VERIFY(bridgeResult->property("visible").toBool());
        QCOMPARE(bridgeResult->property("message").toString(),QStringLiteral("Fixture installation steps"));
        QVERIFY(QMetaObject::invokeMethod(bridgeResult,"close"));
        auto *exportResult = pane->findChild<QObject *>(QStringLiteral("fullContactExportResult"));
        QVERIFY(exportResult);
        model.fullExportFinished("Thunderbird bridge is disconnected.",false);
        QTRY_VERIFY(exportResult->property("visible").toBool());
        QCOMPARE(exportResult->property("iconName").toString(),QStringLiteral("error"));
        QVERIFY(QMetaObject::invokeMethod(exportResult,"close"));
        auto *reviewDot = reviewButton->findChild<QObject *>(QStringLiteral("reviewContactsDot"));
        QVERIFY(reviewDot);
        QVERIFY(reviewDot->property("visible").toBool());
        QVERIFY(reviewButton->property("enabled").toBool());
        QVERIFY(!reviewDialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(reviewButton, "clicked"));
        QTRY_VERIFY(reviewDialog->property("visible").toBool());
        auto *reviews = reviewDialog->findChild<QObject *>(QStringLiteral("contactConflictRepeater"));
        QVERIFY(reviews);
        QCOMPARE(reviews->property("count").toInt(), 1);
        model.resolve(model.reviews().first().toMap().value("key").toString(), "ignore", "", "", "");
        QTRY_VERIFY(model.reviews().isEmpty());
        QTRY_VERIFY(!reviewButton->property("enabled").toBool());
        QTRY_VERIFY(!reviewDot->property("visible").toBool());
        QTRY_COMPARE(reviews->property("count").toInt(), 0);
        QSqlQuery client(global_DBObjects.getDb());
        QVERIFY(client.exec("INSERT INTO clients(id,client_name) VALUES('review-stage-client','Review Test Company')"));
        provider.clientReview=true;model.syncNow();QTRY_VERIFY(!model.busy());
        QTRY_VERIFY(reviewDialog->property("clientStage").toBool());
        QTRY_COMPARE(reviews->property("count").toInt(),1);
        auto *useLeftClient=findVisualChild(window->contentItem(),QStringLiteral("useLeftClientButton"));
        auto *useRightClient=findVisualChild(window->contentItem(),QStringLiteral("useRightClientButton"));
        auto *useBothClients=findVisualChild(window->contentItem(),QStringLiteral("useBothClientsButton"));
        QVERIFY(useLeftClient);QVERIFY(useLeftClient->isVisible());QVERIFY(useLeftClient->isEnabled());
        QVERIFY(useRightClient);QVERIFY(useRightClient->isVisible());QVERIFY(useRightClient->isEnabled());
        QVERIFY(useBothClients);QVERIFY(useBothClients->isVisible());QVERIFY(useBothClients->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(useRightClient,"clicked"));
        QTRY_VERIFY(!reviewDialog->property("clientStage").toBool());
        QTRY_VERIFY(!model.busy());QCOMPARE(model.reviews().size(),1);
        QVERIFY(model.reviews().first().toMap().value("reason").toString().contains("missing"));
        model.resolve(model.reviews().first().toMap().value("key").toString(),"ignore","","","");
        QTRY_COMPARE(reviews->property("count").toInt(),0);
        QVERIFY(client.exec("DELETE FROM clients WHERE id='review-stage-client'"));
        QVERIFY(reviewDialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(reviewDialog, "close"));
        QTRY_VERIFY(!reviewDialog->property("visible").toBool());
        QTest::qWait(60);
        assertClean("native contact conflict review dialog");
        pane.reset();model.setOption("enabled",false);
    }

    // The Settings "View Options" checks and the app menu's View group drive the
    // same controller properties, so they have to agree in both directions: a
    // click in Settings has to move the menu's check mark, and a menu toggle has
    // to move the Settings row — including after the row has been clicked once,
    // which writes `checked` on the control directly.
    void test_02c_viewOptionChecksTrackTheController()
    {
        clearWarnings();
        nav("selectSection", "settings");
        QQuickWindow* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        QQuickItem* settings = findVisualChild(window->contentItem(), QStringLiteral("settingsPage"));
        QQuickItem* pages = findVisualChild(window->contentItem(), QStringLiteral("settingsPages"));
        QVERIFY(settings);
        QVERIFY(pages);
        QVariant selected;
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 8)));
        QVERIFY(selected.toBool());
        QTRY_COMPARE_WITH_TIMEOUT(pages->property("currentIndex").toInt(), 8, 1000);

        QObject* box = root->findChild<QObject*>(QStringLiteral("viewOptionShowClosedBox"));
        QVERIFY2(box, "View Options 'Show closed projects' checkbox not found");

        const bool orig = c->showClosedProjects();

        // Settings -> controller (what the checkbox click does).
        QVERIFY(QMetaObject::invokeMethod(box, "click"));
        QTRY_COMPARE_WITH_TIMEOUT(c->showClosedProjects(), !orig, 1000);

        // Controller -> Settings (what the View menu does), after the click.
        c->setShowClosedProjects(orig);
        QTRY_COMPARE_WITH_TIMEOUT(box->property("checked").toBool(), orig, 1000);

        c->setShowClosedProjects(!orig);
        QTRY_COMPARE_WITH_TIMEOUT(box->property("checked").toBool(), !orig, 1000);

        c->setShowClosedProjects(orig);
        QTRY_COMPARE_WITH_TIMEOUT(box->property("checked").toBool(), orig, 1000);

        // The View menu's own check marks are bound to the same properties, so
        // they have to follow a change made from Settings, and the Settings row
        // has to follow a change made from the menu.
        QObject* appMenu = root->findChild<QObject*>(QStringLiteral("appMenu"));
        QVERIFY2(appMenu, "app menu not found");
        QCOMPARE(menuToggleState(appMenu, QStringLiteral("toggle_closed")), orig);

        QVERIFY(QMetaObject::invokeMethod(appMenu, "_act",
                                          Q_ARG(QVariant, QStringLiteral("toggle_closed"))));
        QTRY_COMPARE_WITH_TIMEOUT(c->showClosedProjects(), !orig, 1000);
        QTRY_COMPARE_WITH_TIMEOUT(box->property("checked").toBool(), !orig, 1000);
        QCOMPARE(menuToggleState(appMenu, QStringLiteral("toggle_closed")), !orig);

        QVERIFY(QMetaObject::invokeMethod(box, "click"));
        QTRY_COMPARE_WITH_TIMEOUT(c->showClosedProjects(), orig, 1000);
        QCOMPARE(menuToggleState(appMenu, QStringLiteral("toggle_closed")), orig);

        // "Show resolved items" reads the same way in both places, but the
        // controller property (newAndAssignedOnly) is its inverse — so this row
        // is the one that binds through a negation, in both directions.
        QObject* resolved = root->findChild<QObject*>(QStringLiteral("viewOptionShowResolvedBox"));
        QVERIFY2(resolved, "View Options 'Show resolved items' checkbox not found");
        const bool showResolved = !c->newAndAssignedOnly();
        QCOMPARE(resolved->property("checked").toBool(), showResolved);
        QCOMPARE(menuToggleState(appMenu, QStringLiteral("toggle_resolved")), showResolved);

        QVERIFY(QMetaObject::invokeMethod(appMenu, "_act",
                                          Q_ARG(QVariant, QStringLiteral("toggle_resolved"))));
        QTRY_COMPARE_WITH_TIMEOUT(c->newAndAssignedOnly(), showResolved, 1000);
        QTRY_COMPARE_WITH_TIMEOUT(resolved->property("checked").toBool(), !showResolved, 1000);

        QVERIFY(QMetaObject::invokeMethod(resolved, "click"));
        QTRY_COMPARE_WITH_TIMEOUT(c->newAndAssignedOnly(), !showResolved, 1000);
        QCOMPARE(menuToggleState(appMenu, QStringLiteral("toggle_resolved")), showResolved);
        assertClean("view option checkbox sync");
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
            auto *window = qobject_cast<QQuickWindow*>(root);
            auto *first = findVisualChild(window->contentItem(), "Item NumberInput");
            QVERIFY(first);
            auto focusedWithin = [window](const QString& name) {
                for (auto *item = window->activeFocusItem(); item; item = item->parentItem())
                    if (item->objectName() == name) return true;
                return false;
            };
            first->forceActiveFocus();
            const QStringList order = {"Item NumberInput", "TypeInput", "Item NameInput",
                                       "ItemDescriptionInput", "Identified ByInput"};
            for (const QString& name : order) {
                QVERIFY2(focusedWithin(name), qPrintable("Tab focus missed " + name));
                QTest::keyClick(window, Qt::Key_Tab);
            }
        }
    }

    // Notes and tracker items fill the whole content area, and the breadcrumb
    // only says "Note" / "Item" — so the project badge is the only thing on
    // either page naming the project the record belongs to. Check it resolves
    // the project rather than sitting there blank, on both pages.
    void test_03b_noteAndItemPagesNameTheirProject()
    {
        QQuickWindow* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);

        const QString pid = projectWithNote();
        QVERIFY2(!pid.isEmpty(), "no seeded project to open");
        const QString number = c->projectNumberForId(pid);
        QVERIFY2(!number.isEmpty(), "seeded project has no project number");

        clearWarnings();
        nav("selectSection", "projects");
        nav("openProject", pid);
        c->setProjectFilter(pid);
        const QString noteId = c->projectNoteIdAtRow(0);
        QVERIFY2(!noteId.isEmpty(), "no seeded note to open");
        nav("openNote", 0, noteId, pid);

        QQuickItem* noteChip = findVisualChild(window->contentItem(), QStringLiteral("projectChip"));
        QVERIFY(noteChip);
        QCOMPARE(noteChip->property("projectId").toString(), pid);
        QCOMPARE(noteChip->property("projectNumber").toString(), number);
        QVERIFY(noteChip->property("linked").toBool());
        nav("goBack");
        nav("goBack");
        assertClean("note project badge");

        clearWarnings();
        nav("selectSection", "items");
        c->refreshAllItems();
        const QString itemId = c->allItemIdAtRow(0);
        QVERIFY2(!itemId.isEmpty(), "no seeded tracker item to open");
        nav("openItem", itemId);

        // The item page learns its project from the record, not from the caller,
        // so the badge is only right if _reload() propagated it.
        QQuickItem* itemChip = findVisualChild(window->contentItem(), QStringLiteral("projectChip"));
        QVERIFY(itemChip);
        const QString itemProject = itemChip->property("projectId").toString();
        QVERIFY2(!itemProject.isEmpty(), "item badge resolved no project");
        QCOMPARE(itemChip->property("projectNumber").toString(), c->projectNumberForId(itemProject));
        QVERIFY(itemChip->property("linked").toBool());
        assertClean("item project badge");
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

    // Schedule Meeting: the review loads a meeting type against a real
    // project and binds its fields and attendee pane without QML warnings.
    // Closing it releases the review. Nothing is handed to a calendar.
    void test_08_scheduleMeetingDialog()
    {
        PN::Comm::MeetingTemplate meetingType;
        meetingType.id = QStringLiteral("smoke-meeting");
        meetingType.name = QStringLiteral("Smoke Meeting");
        meetingType.subject = QStringLiteral("{{ project.number }} smoke");
        meetingType.agenda = QStringLiteral("<p>Agenda for {{ project.name }}</p>");
        QVERIFY(PN::Comm::MeetingTemplateStore().save(meetingType).ok());
        QVERIFY(!c->meetingTemplateChoices().isEmpty());

        const QString pid = firstId(c->projectsListModel());
        QVERIFY2(!pid.isEmpty(), "no seeded project to schedule from");
        clearWarnings();
        nav("selectSection", "projects");
        const QList<QObject*> dialogs = root->findChildren<QObject*>(QStringLiteral("scheduleMeetingDialog"));
        QVERIFY(!dialogs.isEmpty());
        QObject* dialog = dialogs.constFirst();
        dialog->setProperty("projectId", pid);
        dialog->setProperty("templateId", meetingType.id);
        QVERIFY(QMetaObject::invokeMethod(dialog, "open"));
        QTRY_VERIFY_WITH_TIMEOUT(dialog->property("ready").toBool(), 5000);
        const QVariantMap review = c->meetingReview();
        QVERIFY(review.value(QStringLiteral("subject")).toString().endsWith(QStringLiteral(" smoke")));
        QCOMPARE(review.value(QStringLiteral("durationMinutes")).toInt(), 60);
        assertClean("schedule meeting dialog");

        // iCloud cannot deep-link to the event it creates.  Its successful
        // handoff must therefore leave the review open so Copy Attendees is
        // still available instead of dismissing the only attendee control.
        QVariantMap iCloudReview = review;
        iCloudReview.insert(QStringLiteral("backend"), QStringLiteral("icloud"));
        dialog->setProperty("review", iCloudReview);
        QVERIFY(QMetaObject::invokeMethod(c, "meetingHandoffFinished",
                                          Q_ARG(bool, true), Q_ARG(QString, QString())));
        QVERIFY(dialog->property("visible").toBool());
        QVERIFY(dialog->property("draftCreated").toBool());
        QObject *copyAttendees = dialog->findChild<QObject*>(QStringLiteral("copyMeetingAttendeesButton"));
        QVERIFY(copyAttendees);
        QVERIFY(copyAttendees->property("visible").toBool());
        QVERIFY(copyAttendees->property("enabled").toBool());

        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTRY_VERIFY_WITH_TIMEOUT(!c->meetingReview().value(QStringLiteral("ready")).toBool(), 1000);
        QVERIFY(PN::Comm::MeetingTemplateStore().remove(meetingType.id).ok());
        assertClean("schedule meeting dialog closed");
    }

    // Every text-editing surface carries the shared text context menu
    // (SpellCheckField), so right-click offers the same Cut/Copy/Paste/Select
    // All everywhere instead of falling back to whatever the control does on
    // its own. Scans what the earlier tests have already built: every page,
    // the settings panes, the help viewer and the dialogs they opened.
    void test_09_textEditorsCarryTheStandardContextMenu()
    {
        nav("selectSection", "projects");
        nav("handleMenuAction", "preferences");
        nav("selectSection", "search");
        nav("openHelp", "index.md");

        QStringList missing;
        QStringList doubled;
        int scanned = 0;
        for (QQuickItem* item : root->findChildren<QQuickItem*>()) {
            if (!isTextEditor(item))
                continue;
            // A fixed-list ComboField disables its box so the whole control
            // opens the list; there is no text of its own to act on.
            if (!item->isEnabled())
                continue;
            ++scanned;
            bool hasMenu = false;
            for (QQuickItem* child : item->findChildren<QQuickItem*>()) {
                if (child->property("spellCheckEnabled").isValid()) {
                    hasMenu = true;
                    break;
                }
            }
            if (hasMenu) {
                // Qt's own deferred context menu must be cleared, or it would
                // take over from the second right-click on.
                const QQmlProperty builtIn(item, QStringLiteral("ContextMenu.menu"), qmlContext(item));
                if (builtIn.isValid() && builtIn.read().value<QObject*>() != nullptr)
                    doubled.append(describe(item));
                continue;
            }
            missing.append(describe(item));
        }
        missing.removeDuplicates();
        doubled.removeDuplicates();
        // A scan that reached nothing would pass while proving nothing.
        QVERIFY2(scanned >= 10, qPrintable(QStringLiteral("only %1 text editors reached").arg(scanned)));
        QVERIFY2(missing.isEmpty(),
                 qPrintable(QStringLiteral("text editors without the shared context menu: %1")
                                .arg(missing.join(QStringLiteral(", ")))));
        QVERIFY2(doubled.isEmpty(),
                 qPrintable(QStringLiteral("text editors that kept Qt's own context menu too: %1")
                                .arg(doubled.join(QStringLiteral(", ")))));
    }

    void cleanupTestCase()
    {
        delete engine;   // tear the window down before the app object goes away
        engine = nullptr;
    }

private:
    // Controls' TextField/TextArea are QML composite types (and pages subclass
    // them further), so their class names are generated. Identify an editor by
    // the editing API instead: a plain Text has none of these.
    static bool isTextEditor(QQuickItem* item)
    {
        return item && item->property("canPaste").isValid()
            && item->property("selectedText").isValid()
            && item->property("cursorPosition").isValid();
    }

    static QString describe(QQuickItem* item)
    {
        QString where = QString::fromLatin1(item->metaObject()->className());
        if (QQmlContext* context = qmlContext(item))
            where += QStringLiteral(" in ") + context->baseUrl().fileName();
        if (!item->objectName().isEmpty())
            where += QStringLiteral(" (") + item->objectName() + QLatin1Char(')');
        return where;
    }

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

    // The `on` flag the app menu's View group currently shows for a toggle
    // action — i.e. whether that row would draw a check mark right now.
    static bool menuToggleState(QObject* appMenu, const QString& action)
    {
        const QVariantList groups = appMenu->property("groups").toList();
        for (const QVariant& g : groups) {
            const QVariantList items = g.toMap().value(QStringLiteral("items")).toList();
            for (const QVariant& i : items) {
                const QVariantMap item = i.toMap();
                if (item.value(QStringLiteral("action")).toString() == action)
                    return item.value(QStringLiteral("on")).toBool();
            }
        }
        qWarning() << "no menu item with action" << action;
        return false;
    }

    static QString firstId(QAbstractItemModel* m)
    {
        if (!m || m->rowCount() < 1) return {};
        return m->data(m->index(0, 0)).toString();
    }

    QString projectWithNote()
    {
        auto *projects = c->projectsListModel();
        for (int row = 0; projects && row < projects->rowCount(); ++row) {
            const QString id = projects->data(projects->index(row, 0)).toString();
            c->setProjectFilter(id);
            if (!c->projectNoteIdAtRow(0).isEmpty())
                return id;
        }
        return {};
    }
};

int runUiSmokeTests(int argc, char** argv)
{
    UiSmokeTest t;
    return QTest::qExec(&t, argc, argv);
}

#include "tst_uismoke.moc"

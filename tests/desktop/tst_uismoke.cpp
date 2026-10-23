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
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>

#include "PluginUiController.h"
#include "ProjectNotesPlugins/PluginRegistry.h"
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
#include "FolderManager.h"
#include "ProjectNotesIntegrations/MeetingTemplateStore.h"
#include "ProjectNotesContacts/ContactSyncSettingsModel.h"
#include "databaseobjects.h"
#include <QQmlComponent>
#include <QJSValue>
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

    void test_00_keyboardListNavigation()
    {
        clearWarnings();
        QQmlComponent component(engine);
        component.setData(R"(
            import QtQuick
            import ProjectNotesDesktop
            KeyboardListView {
                width: 240; height: 150
                model: 50
                property bool hideOdd: false
                header: Item { width: 240; height: 40 }
                delegate: Rectangle {
                    required property int index
                    width: 240
                    visible: !ListView.view.hideOdd || index % 2 === 0
                    height: visible ? 30 : 0
                    color: index % 2 ? "white" : "lightgray"
                }
            }
        )", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* list = qobject_cast<QQuickItem*>(object.data());
        QVERIFY(list);
        QQuickWindow window;
        window.resize(240, 150);
        list->setParentItem(window.contentItem());
        window.show();
        list->forceActiveFocus();
        QTest::qWait(50);
        QCOMPARE(list->property("currentIndex").toInt(), 0);
        const qreal initialContentY = list->property("contentY").toReal();
        QTest::keyClick(&window, Qt::Key_Down);
        QCOMPARE(list->property("currentIndex").toInt(), 1);
        QTest::keyClick(&window, Qt::Key_Up);
        QCOMPARE(list->property("currentIndex").toInt(), 0);
        QTest::keyClick(&window, Qt::Key_PageDown);
        QCOMPARE(list->property("currentIndex").toInt(), 4);
        QTRY_VERIFY(list->property("contentY").toReal() > initialContentY);
        QTest::keyClick(&window, Qt::Key_PageUp);
        QCOMPARE(list->property("currentIndex").toInt(), 0);
        QTest::keyClick(&window, Qt::Key_Up);
        QCOMPARE(list->property("currentIndex").toInt(), 0);
        for (int i = 0; i < 15; ++i) QTest::keyClick(&window, Qt::Key_PageDown);
        QCOMPARE(list->property("currentIndex").toInt(), 49);
        QTest::keyClick(&window, Qt::Key_Down);
        QCOMPARE(list->property("currentIndex").toInt(), 49);
        QTest::qWait(50);
        list->setFocus(false);
        window.contentItem()->forceActiveFocus();
        QVERIFY(!list->hasActiveFocus());
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(120, 45));
        QVERIFY(list->hasActiveFocus());
        const int clickedRow = list->property("currentIndex").toInt();
        QVERIFY(clickedRow < 49);
        QTest::keyClick(&window, Qt::Key_Down);
        QCOMPARE(list->property("currentIndex").toInt(), clickedRow + 1);
        list->setProperty("hideOdd", true);
        list->setProperty("currentIndex", 0);
        QTest::qWait(50);
        QTest::keyClick(&window, Qt::Key_Down);
        QCOMPARE(list->property("currentIndex").toInt(), 2);
        QTest::keyClick(&window, Qt::Key_Up);
        QCOMPARE(list->property("currentIndex").toInt(), 0);
        QTest::keyClick(&window, Qt::Key_PageDown);
        QCOMPARE(list->property("currentIndex").toInt(), 8);
        list->setProperty("model", 0);
        QTest::keyClick(&window, Qt::Key_PageDown);
        QCOMPARE(list->property("currentIndex").toInt(), -1);
        assertClean("list keyboard navigation");
    }

    void test_00_keyboardDropdownNavigation()
    {
        clearWarnings();
        QQmlComponent component(engine);
        component.setData(R"(
            import QtQuick
            import QtQuick.Controls.Basic
            import ProjectNotesDesktop
            KeyboardComboBox {
                width: 240; height: 30
                model: 50
                popup: Popup {
                    y: 30; width: 240; height: 150
                    contentItem: ListView {
                        implicitHeight: contentHeight
                        model: parent.visible ? control.delegateModel : null
                    }
                }
                id: control
                delegate: ItemDelegate { width: 240; height: 30; text: index }
            }
        )", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* combo = qobject_cast<QQuickItem*>(object.data());
        QVERIFY(combo);
        QQuickWindow window;
        window.resize(240, 240);
        combo->setParentItem(window.contentItem());
        window.show();
        combo->forceActiveFocus();
        QTest::qWait(50);
        QTest::keyClick(&window, Qt::Key_PageDown);
        QVERIFY(combo->property("highlightedIndex").toInt() > 1);
        QCOMPARE(combo->property("currentIndex").toInt(), 0);
        QTest::keyClick(&window, Qt::Key_PageUp);
        QCOMPARE(combo->property("highlightedIndex").toInt(), 0);
        QTest::keyClick(&window, Qt::Key_Down);
        QCOMPARE(combo->property("highlightedIndex").toInt(), 1);
        QTest::keyClick(&window, Qt::Key_Up);
        QCOMPARE(combo->property("highlightedIndex").toInt(), 0);
        assertClean("dropdown keyboard navigation");
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
        menu->setProperty("contactId", "");
        QVERIFY(menu->property("canContactEmail").toBool());
        QVERIFY(!menu->property("canProjectEmail").toBool());
        // Contact cards: the contact row itself, or the person behind a team
        // member or attendee row; other tables never offer them.
        QVERIFY(menu->property("canContactCard").toBool());
        QCOMPARE(menu->property("contactCardId").toString(), projectId);
        for (const char *table : {"project_people", "meeting_attendees"}) {
            menu->setProperty("recordTable", table);
            menu->setProperty("contactId", "");
            QVERIFY(!menu->property("canContactCard").toBool());
            menu->setProperty("contactId", "person-1");
            QCOMPARE(menu->property("contactCardId").toString(), QString("person-1"));
        }
        menu->setProperty("recordTable", "item_tracker");
        QVERIFY(!menu->property("canContactCard").toBool());
        menu->setProperty("contactId", "");
        menu->setProperty("recordTable", "project_locations");
        menu->setProperty("emailProjectId", projectId);
        menu->setProperty("locationPath", "https://example.test/file.pdf");
        QVERIFY(menu->property("canShareLocation").toBool());
        QVERIFY(menu->property("canShareLink").toBool());
        menu->setProperty("locationPath", "/tmp/file.pdf");
        QVERIFY(!menu->property("canShareLink").toBool());
        // Primary project folder actions: File Folder locations only, and the
        // set / clear / hand-back items follow the stored choice.
        {
            const QString folderId = "smoke-primary-folder";
            QSqlQuery q(global_DBObjects.getDb());
            QVERIFY(q.exec("INSERT INTO project_locations(id, project_id, location_type, location_description, full_path, deleted) "
                           "VALUES('" + folderId + "', '" + projectId + "', 'File Folder', 'Smoke Folder', '/tmp/smoke', 0)"));
            const QString before = c->primaryProjectLocationId(projectId);
            const bool beforeChosen = c->projectFolderChosenByUser(projectId);
            menu->setProperty("recordId", folderId);
            menu->setProperty("locationType", "PDF File");
            QVERIFY(!menu->property("canProjectFolder").toBool());
            menu->setProperty("locationType", "File Folder");
            QVERIFY(menu->property("canProjectFolder").toBool());
            QVERIFY(c->setPrimaryProjectLocation(projectId, folderId));
            QVERIFY(menu->property("_isProjectFolder").toBool());
            QVERIFY(menu->property("_projectFolderChosen").toBool());
            QVERIFY(c->setPrimaryProjectLocation(projectId, ""));
            QVERIFY(!menu->property("_isProjectFolder").toBool());
            QVERIFY(c->letFileFinderChooseProjectFolder(projectId));
            QVERIFY(!menu->property("_projectFolderChosen").toBool());
            QVERIFY(q.exec("DELETE FROM project_locations WHERE id = '" + folderId + "'"));
            q.prepare("UPDATE projects SET primary_location_id = ?, primary_location_manual = ? WHERE id = ?");
            q.addBindValue(before.isEmpty() ? QVariant() : QVariant(before));
            q.addBindValue(beforeChosen ? 1 : 0);
            q.addBindValue(projectId);
            QVERIFY(q.exec());
            menu->setProperty("locationType", "");
            menu->setProperty("recordId", projectId);
        }
        menu->setProperty("recordTable", "meeting_attendees");
        menu->setProperty("contactId", "person");
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
        const auto people = c->contactList();
        QVERIFY(!people.isEmpty());
        QVERIFY(QMetaObject::invokeMethod(dialog, "openContact",
            Q_ARG(QVariant, people.first().toMap().value("id"))));
        QTRY_VERIFY_WITH_TIMEOUT(dialog->property("ready").toBool(), 5000);
        QVERIFY(dialog->property("plainContact").toBool());
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
                                          Q_ARG(QVariant, 8)));
        QVERIFY(selected.toBool());
        QTRY_COMPARE_WITH_TIMEOUT(pages->property("currentIndex").toInt(), 8, 1000);
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
                                          Q_ARG(QVariant, 9)));
        QVERIFY(selected.toBool());
        QTRY_COMPARE_WITH_TIMEOUT(pages->property("currentIndex").toInt(), 9, 1000);
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
        // Discarding the blank type returns to a seeded standard template.
        QVERIFY(model->property("hasDraft").toBool());
        QCOMPARE(model->property("templates").toList().size(), 12);
        assertClean("blank meeting type discarded");

        // A named draft with no subject is kept but blocks leaving the tab
        // behind the themed validation dialog.
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 9)));
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
            "projects", "items", "contacts", "companies", "search", "settings", "help", "projects"
        };
        for (const QString& s : sections) {
            clearWarnings();
            nav("selectSection", s);
            assertClean(qPrintable(QStringLiteral("selectSection('%1')").arg(s)));
        }
    }

    void test_01b_openPagesReceiveNavigationKeys()
    {
        clearWarnings();
        auto* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        window->show();
        auto* stack = findVisualChild(window->contentItem(), "contentStack");
        QVERIFY(stack);
        for (const QString& section : {QString("projects"), QString("items"),
                                       QString("contacts"), QString("companies"), QString("projects")}) {
            nav("selectSection", section);
            auto* page = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
            QVERIFY(page);
            QVERIFY(window->activeFocusItem() == page || page->isAncestorOf(window->activeFocusItem()));
            QVariant result;
            QVERIFY(QMetaObject::invokeMethod(root, "_pageScrollTarget", Q_RETURN_ARG(QVariant, result)));
            auto* list = qobject_cast<QQuickItem*>(result.value<QObject*>());
            QVERIFY(list);
            const QVariant originalModel = list->property("model");
            list->setProperty("model", 100);
            list->setProperty("currentIndex", 0);
            QTest::qWait(50);
            QTest::keyClick(window, Qt::Key_Down);
            QCOMPARE(list->property("currentIndex").toInt(), 1);
            QTest::keyClick(window, Qt::Key_Up);
            QCOMPARE(list->property("currentIndex").toInt(), 0);
            QTest::keyClick(window, Qt::Key_PageDown);
            QVERIFY(list->property("currentIndex").toInt() > 1);
            QTest::keyClick(window, Qt::Key_PageUp);
            QCOMPARE(list->property("currentIndex").toInt(), 0);
            list->setProperty("model", originalModel);
            assertClean(qPrintable("keyboard navigation on " + section));
        }
        window->hide();
    }

    void test_01c_plainPageScrollingAndEditorKeys()
    {
        clearWarnings();
        nav("selectSection", "projects");
        auto* window = qobject_cast<QQuickWindow*>(root);
        auto* stack = findVisualChild(window->contentItem(), "contentStack");
        auto* page = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
        QVERIFY(page);
        QQmlComponent component(engine);
        component.setData(R"(
            import QtQuick
            Flickable {
                width: 300; height: 200
                contentWidth: width; contentHeight: 2000
                boundsBehavior: Flickable.StopAtBounds
                TextInput { objectName: "scrollTestEditor"; width: 150; height: 30; text: "editable" }
            }
        )", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* flick = qobject_cast<QQuickItem*>(object.data());
        flick->setParentItem(page);
        window->show();
        flick->forceActiveFocus();
        QTest::qWait(50);
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(flick->property("contentY").toReal(), 40.0);
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(flick->property("contentY").toReal(), 0.0);
        QTest::keyClick(window, Qt::Key_PageDown);
        QCOMPARE(flick->property("contentY").toReal(), 180.0);
        QTest::keyClick(window, Qt::Key_PageUp);
        QCOMPARE(flick->property("contentY").toReal(), 0.0);
        auto* editor = object->findChild<QQuickItem*>("scrollTestEditor");
        QVERIFY(editor);
        editor->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_PageDown);
        QCOMPARE(flick->property("contentY").toReal(), 0.0);
        QTest::keyClick(window, Qt::Key_Return);
        QCOMPARE(stack->property("depth").toInt(), 1);
        QTest::keyClick(window, Qt::Key_X);
        QVERIFY(editor->property("text").toString().contains('x'));
        window->hide();
        assertClean("plain scrolling page and editor keys");
    }

    // The contact card's ContactDragArea (vCard drag-out) takes the card's
    // presses, so it must still open the contact on click and its menu on
    // right-click.
    void test_01c2_contactCardClickAndRightClick()
    {
        clearWarnings();
        auto* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        window->show();
        window->requestActivate();
        auto* stack = findVisualChild(window->contentItem(), "contentStack");
        QVERIFY(stack);
        nav("selectSection", "contacts");
        QTRY_VERIFY(!stack->property("busy").toBool());
        auto* page = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
        QVERIFY(page);
        auto* area = findVisualChild(page, "contactDragArea");
        QVERIFY(area);
        auto* card = area->parentItem();
        const QString id = card->property("pid").toString();
        QVERIFY(!id.isEmpty());
        QCOMPARE(area->property("contactId").toString(), id);
        // Left of the card, on the avatar: away from the kebab button.
        const QPoint point = card->mapToScene(QPointF(22, card->height() / 2)).toPoint();
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);
        QTRY_COMPARE(stack->property("depth").toInt(), 2);
        QTRY_VERIFY(!stack->property("busy").toBool());
        auto* detail = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
        QVERIFY(detail);
        QCOMPARE(detail->property("contactId").toString(), id);
        QTest::keyClick(window, Qt::Key_Left, Qt::AltModifier);
        QTRY_COMPARE(stack->property("depth").toInt(), 1);
        QTRY_VERIFY(!stack->property("busy").toBool());
        page->setProperty("_ctxId", QString());
        QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, point);
        QTRY_COMPARE(page->property("_ctxId").toString(), id);
        bool menuClosed = false;
        for (QObject* object : page->findChildren<QObject*>()) {
            if (object->property("contactCardId").isValid() && object->property("visible").toBool()) {
                QCOMPARE(object->property("contactCardId").toString(), id);
                QVERIFY(object->property("canContactCard").toBool());
                QMetaObject::invokeMethod(object, "close");
                menuClosed = true;
            }
        }
        QVERIFY(menuClosed);
        QTest::qWait(50);
        window->hide();
        assertClean("contact card click and right-click");
    }

    void test_01d_enterOpensSelectedRecordsAndKeyboardBack()
    {
        clearWarnings();
        auto* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        window->show();
        window->requestActivate();
        auto* stack = findVisualChild(window->contentItem(), "contentStack");
        QVERIFY(stack);
        struct Case { const char* section; const char* rowId; const char* detailId; };
        const Case cases[] = {
            {"projects", "projId", "projectId"}, {"items", "iid", "itemId"},
            {"contacts", "pid", "contactId"}, {"companies", "cid", "companyId"}
        };
        for (const auto& entry : cases) {
            nav("selectSection", QString::fromLatin1(entry.section));
            auto* page = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
            QVERIFY(page);
            QVariant result;
            QVERIFY(QMetaObject::invokeMethod(root, "_pageScrollTarget", Q_RETURN_ARG(QVariant, result)));
            auto* list = qobject_cast<QQuickItem*>(result.value<QObject*>());
            QVERIFY(list);
            QVERIFY(list->property("count").toInt() > 0);
            list->setProperty("currentIndex", 0);
            QTest::keyClick(window, Qt::Key_Down);
            QTest::qWait(50);
            auto* row = qvariant_cast<QQuickItem*>(list->property("currentItem"));
            QVERIFY(row);
            const QString id = row->property(entry.rowId).toString();
            QVERIFY(!id.isEmpty());
            const int selectedIndex = list->property("currentIndex").toInt();
            QTest::keyClick(window, Qt::Key_Return);
            QTRY_COMPARE(stack->property("depth").toInt(), 2);
            QTRY_VERIFY(!stack->property("busy").toBool());
            auto* detail = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
            QVERIFY(detail);
            QCOMPARE(detail->property(entry.detailId).toString(), id);
            QTest::qWait(50);
            QTest::keyClick(window, Qt::Key_Left, Qt::AltModifier);
            QTRY_COMPARE(stack->property("depth").toInt(), 1);
            QTRY_VERIFY(!stack->property("busy").toBool());
            QCOMPARE(qvariant_cast<QQuickItem*>(stack->property("currentItem")), page);
            QCOMPARE(list->property("currentIndex").toInt(), selectedIndex);
            // Enter also works directly after returning, with focus on the page.
            QTest::keyClick(window, Qt::Key_Enter);
            QTRY_COMPARE(stack->property("depth").toInt(), 2);
            QTRY_VERIFY(!stack->property("busy").toBool());
            detail = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
            QCOMPARE(detail->property(entry.detailId).toString(), id);
            QTest::qWait(50);
            QTest::keyClick(window, Qt::Key_Back);
            QTRY_COMPARE(stack->property("depth").toInt(), 1);
            QTRY_VERIFY(!stack->property("busy").toBool());
            QTest::keyClick(window, Qt::Key_Left, Qt::AltModifier);
            QCOMPARE(stack->property("depth").toInt(), 1);
            assertClean(qPrintable("Enter and Back on " + QString::fromLatin1(entry.section)));
        }
        window->hide();
    }

    void test_01e_railShortcutsAndNavigateMenu()
    {
        clearWarnings();
        auto* window = qobject_cast<QQuickWindow*>(root);
        window->show();
        window->requestActivate();
        QTest::qWait(50);
        auto* stack = findVisualChild(window->contentItem(), "contentStack");
        QVERIFY(stack);
        QObject* menu = root->findChild<QObject*>("appMenu");
        QVERIFY(menu);
        QVariant groupsValue = menu->property("groups");
        const auto groups = groupsValue.value<QJSValue>().toVariant().toList();
        QVariantList destinations;
        int navigateIndex = -1;
        for (int i = 0; i < groups.size(); ++i) {
            const auto group = groups[i].toMap();
            if (group.value("name").toString() == "Navigate") {
                destinations = group.value("items").toList();
                navigateIndex = i;
            }
        }
        QCOMPARE(destinations.size(), 7);
        const QStringList sections = {"projects", "items", "contacts", "companies", "search", "help", "settings"};
        const QStringList labels = {"Projects", "Action Items", "Contacts", "Companies", "Search", "User Guide", "Settings"};
        const QString projectId = projectWithNote();
        QVERIFY(!projectId.isEmpty());
        for (int i = 0; i < sections.size(); ++i) {
            const auto destination = destinations[i].toMap();
            QCOMPARE(destination.value("label").toString(), labels[i]);
            QCOMPARE(destination.value("key").toString(), c->nativeShortcutText(QString("Ctrl+%1").arg(i + 1)));
            auto* button = findVisualChild(window->contentItem(), "rail_" + sections[i]);
            QVERIFY(button);
            QVERIFY(button->property("tooltip").toString().contains(destination.value("key").toString()));
            nav("selectSection", "projects");
            nav("openProject", projectId);
            QTRY_VERIFY(!stack->property("busy").toBool());
            QTest::keyClick(window, Qt::Key(Qt::Key_1 + i), Qt::ControlModifier);
            QTRY_COMPARE(root->property("currentSection").toString(), sections[i]);
            QTRY_COMPARE(stack->property("depth").toInt(), 1);
            QTRY_VERIFY(!stack->property("busy").toBool());
            // The same keys work with the modal hamburger popup open.
            QVERIFY(QMetaObject::invokeMethod(menu, "open"));
            QTRY_VERIFY(menu->property("opened").toBool());
            QTest::keyClick(window, Qt::Key(Qt::Key_1 + i), Qt::ControlModifier);
            QTRY_VERIFY(!menu->property("visible").toBool());
            QCOMPARE(root->property("currentSection").toString(), sections[i]);
            // Menu actions use exactly the same destination identifiers.
            nav("selectSection", "projects");
            QVERIFY(QMetaObject::invokeMethod(menu, "_act", Q_ARG(QVariant, destination.value("action"))));
            QTRY_COMPARE(root->property("currentSection").toString(), sections[i]);
            QTRY_VERIFY(!stack->property("busy").toBool());
            assertClean(qPrintable("rail/menu navigation to " + sections[i]));
        }
        nav("selectSection", "projects");
        QObject* about = root->findChild<QObject*>("aboutDialog");
        QVERIFY(about);
        QVERIFY(QMetaObject::invokeMethod(about, "open"));
        QTRY_VERIFY(about->property("opened").toBool());
        QTest::keyClick(window, Qt::Key_2, Qt::ControlModifier);
        QCOMPARE(root->property("currentSection").toString(), QString("projects"));
        QVERIFY(QMetaObject::invokeMethod(about, "close"));
        QTRY_VERIFY(!about->property("visible").toBool());
        // Open the actual Navigate flyout for visual QA.
        QVERIFY(QMetaObject::invokeMethod(menu, "open"));
        QTRY_VERIFY(menu->property("opened").toBool());
        QVariant allGroupsValue = menu->property("_allGroups");
        const auto allGroups = allGroupsValue.value<QJSValue>().toVariant().toList();
        for (int i = 0; i < allGroups.size(); ++i)
            if (allGroups[i].toMap().value("name").toString() == "Navigate") navigateIndex = i;
        QVERIFY(QMetaObject::invokeMethod(menu, "_activateGroup", Q_ARG(QVariant, navigateIndex)));
        const QImage screenshot = window->grabWindow();
        if (!screenshot.isNull()) screenshot.save("/tmp/projectnotes-navigate-menu.png");
        QTest::keyClick(window, Qt::Key_2, Qt::ControlModifier);
        QTRY_COMPARE(root->property("currentSection").toString(), QString("items"));
        QTRY_VERIFY(!menu->property("visible").toBool());
        QTest::qWait(50);  // let the menu dismissal shield close
        // Preserve the existing Search and Settings aliases.
        QTest::keyClick(window, Qt::Key_K, Qt::ControlModifier);
        QTRY_COMPARE(root->property("currentSection").toString(), QString("search"));
        QTest::keyClick(window, Qt::Key_Comma, Qt::ControlModifier);
        QTRY_COMPARE(root->property("currentSection").toString(), QString("settings"));
        nav("selectSection", "companies");
        QTest::keyClick(window, Qt::Key_F1);
        QTRY_COMPARE(root->property("currentSection").toString(), QString("help"));
        auto* helpPage = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
        QCOMPARE(helpPage->property("topic").toString(), QString("InterfaceOverview/CompaniesListPage.md"));
        // A shortcut while typing uses the same save path as a rail click.
        const QString clientId = c->companyIdAtProxyRow(0);
        QVERIFY(!clientId.isEmpty());
        int clientRow = c->companyRowForId(clientId);
        const QString oldName = c->getCompanyData(clientRow).value("client_name").toString();
        nav("selectSection", "companies");
        nav("openCompany", clientRow, clientId);
        QTRY_VERIFY(!stack->property("busy").toBool());
        auto* name = findVisualChild(window->contentItem(), "Company NameInput");
        QVERIFY(name);
        name->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
        for (char key : QByteArray("Rail Shortcut Saved Client")) QTest::keyClick(window, key);
        QTest::keyClick(window, Qt::Key_1, Qt::ControlModifier);
        QTRY_COMPARE(root->property("currentSection").toString(), QString("projects"));
        QCOMPARE(c->getCompanyData(c->companyRowForId(clientId)).value("client_name").toString(),
                 QString("Rail Shortcut Saved Client"));
        QVERIFY(c->saveCompany(c->companyRowForId(clientId), oldName));
        // Invalid settings drafts keep the user on the current page.
        nav("selectSection", "settings");
        auto* settings = findVisualChild(window->contentItem(), "settingsPage");
        QVERIFY(settings);
        QVariant selected;
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 9)));
        QVERIFY(selected.toBool());
        QObject* model = c->meetingTemplateEditorModel();
        QVERIFY(QMetaObject::invokeMethod(model, "beginNew"));
        model->setProperty("draftName", "Navigation validation draft");
        QTest::keyClick(window, Qt::Key_1, Qt::ControlModifier);
        QCOMPARE(root->property("currentSection").toString(), QString("settings"));
        QObject* validation = root->findChild<QObject*>("meetingValidationDialog");
        QVERIFY(validation);
        QTRY_VERIFY(validation->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(validation, "close"));
        QVERIFY(QMetaObject::invokeMethod(model, "deleteSelected"));
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 0)));
        QVERIFY(selected.toBool());
        nav("selectSection", "projects");
        QTRY_VERIFY(!stack->property("busy").toBool());
        window->hide();
        assertClean("rail shortcuts, modal guard and Navigate flyout");
    }

    void test_01f_sidebarFolderKeyboardNavigation()
    {
        clearWarnings();
        nav("selectSection", "projects");
        auto* window = qobject_cast<QQuickWindow*>(root);
        const QSize originalSize = window->size();
        window->resize(originalSize.width(), 560);
        window->show();
        window->requestActivate();
        auto* stack = findVisualChild(window->contentItem(), "contentStack");
        auto* sidebar = findVisualChild(window->contentItem(), "projectSidebar");
        QVERIFY(stack); QVERIFY(sidebar);
        const bool sidebarWasShown = c->showProjectSidebar();
        c->setShowProjectSidebar(true);
        auto* folders = FolderManager::instance();
        QVERIFY(folders);
        const bool allCollapsed = folders->allProjectsCollapsed();
        const bool uncategorizedCollapsed = folders->uncategorizedCollapsed();
        folders->setAllProjectsCollapsed(false);
        folders->setUncategorizedCollapsed(true);
        const QString projectId = projectWithNote();
        QVERIFY(!projectId.isEmpty());
        const QString folderId = folders->addFolder("Keyboard Folder");
        QVERIFY(!folderId.isEmpty());
        QVERIFY(folders->setProjectFolder(projectId, folderId));
        QVERIFY(folders->setFolderCollapsed(folderId, true));
        QStringList extraFolders;
        for (int i = 0; i < 20; ++i) {
            const QString id = folders->addFolder(QString("Keyboard Empty %1").arg(i));
            QVERIFY(!id.isEmpty());
            QVERIFY(folders->setFolderCollapsed(id, true));
            extraFolders << id;
        }
        const QString headerKey = folderId + "/folder";
        auto expanded = [&]() {
            auto* header = findVisualChild(sidebar, "sidebar_" + headerKey);
            return header && header->parentItem()->property("expanded").toBool();
        };
        QTest::qWait(100);
        sidebar->setProperty("keyboardKey", headerKey);
        sidebar->forceActiveFocus();
        QVERIFY(!expanded());
        // Right/Left expand and collapse a header; Return filters by it.
        QTest::keyClick(window, Qt::Key_Right);
        QTRY_VERIFY(expanded());
        QCOMPARE(sidebar->property("keyboardKey").toString(), headerKey);
        const int allCount = c->folderProjects("").size();
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_COMPARE(c->projectFolderFilter(), QStringList{folderId});
        QCOMPARE(c->projectsListModel()->rowCount(), 1);
        QCOMPARE(c->folderProjects("").size(), allCount);
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(c->projectFolderFilter().isEmpty());
        QVERIFY(expanded());
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(sidebar->property("keyboardKey").toString(), folderId + "/project/" + projectId);
        QTest::keyClick(window, Qt::Key_Enter);
        QTRY_COMPARE(stack->property("depth").toInt(), 2);
        QTRY_VERIFY(!stack->property("busy").toBool());
        auto* detail = qvariant_cast<QQuickItem*>(stack->property("currentItem"));
        QCOMPARE(detail->property("projectId").toString(), projectId);
        QTest::keyClick(window, Qt::Key_Left, Qt::AltModifier);
        QTRY_COMPARE(stack->property("depth").toInt(), 1);
        QTRY_VERIFY(!stack->property("busy").toBool());
        sidebar->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(sidebar->property("keyboardKey").toString(), headerKey);
        QTest::keyClick(window, Qt::Key_Left);
        QTRY_VERIFY(!expanded());
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(sidebar->property("keyboardKey").toString(), extraFolders.first() + "/folder");
        // Expanding an empty folder leaves its header highlighted.
        QTest::keyClick(window, Qt::Key_Right);
        QCOMPARE(sidebar->property("keyboardKey").toString(), extraFolders.first() + "/folder");
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(sidebar->property("keyboardKey").toString(), extraFolders[1] + "/folder");
        for (int i = 0; i < 100; ++i) QTest::keyClick(window, Qt::Key_Down);
        const QString lastKey = sidebar->property("keyboardKey").toString();
        QVERIFY(lastKey.startsWith("all/project/"));
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(sidebar->property("keyboardKey").toString(), lastKey);
        auto* scroll = findVisualChild(sidebar, "projectFolderScroll");
        QVERIFY(scroll);
        auto* flick = qvariant_cast<QQuickItem*>(scroll->property("contentItem"));
        QVERIFY(flick);
        QVERIFY(flick->property("contentY").toReal() > 0);
        for (int i = 0; i < 100; ++i) QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(sidebar->property("keyboardKey").toString(), headerKey);
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(sidebar->property("keyboardKey").toString(), headerKey);
        // Clicking a folder also establishes the keyboard navigation target.
        qvariant_cast<QQuickItem*>(stack->property("currentItem"))->forceActiveFocus();
        auto* header = findVisualChild(sidebar, "sidebar_" + headerKey);
        QVERIFY(header);
        const QPoint click = header->mapToScene(QPointF(header->width() / 2, header->height() / 2)).toPoint();
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, click);
        QVERIFY(sidebar->hasActiveFocus());
        QCOMPARE(sidebar->property("keyboardKey").toString(), headerKey);
        // Clicking the name filters; only the chevron collapses.
        QTRY_COMPARE(c->projectFolderFilter(), QStringList{folderId});
        QVERIFY(!expanded());
        auto* chevron = findVisualChild(sidebar, "sidebar_" + headerKey + "/chevron");
        QVERIFY(chevron);
        const QPoint chevronClick = chevron->mapToScene(QPointF(chevron->width() / 2, chevron->height() / 2)).toPoint();
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, chevronClick);
        QTRY_VERIFY(expanded());
        QCOMPARE(c->projectFolderFilter(), QStringList{folderId});
        c->clearProjectFolderFilter();
        QVERIFY(c->projectFolderFilter().isEmpty());
        // The header menu's folder dialog edits name, icon and color together.
        auto* editDialog = root->findChild<QObject*>("folderEditDialog");
        QVERIFY(editDialog);
        QVariantMap folderMap;
        for (const QVariant& f : folders->folders())
            if (f.toMap().value("id").toString() == folderId) folderMap = f.toMap();
        QVERIFY(QMetaObject::invokeMethod(editDialog, "openFor", Q_ARG(QVariant, folderMap)));
        QTRY_VERIFY(editDialog->property("opened").toBool());
        auto* nameField = editDialog->findChild<QObject*>("folderEditName");
        QVERIFY(nameField);
        nameField->setProperty("text", "Keyboard Folder Renamed");
        editDialog->setProperty("_icon", "bolt");
        QVERIFY(QMetaObject::invokeMethod(editDialog, "_save"));
        QTRY_VERIFY(!editDialog->property("visible").toBool());
        QCOMPARE(folders->folderName(folderId), QString("Keyboard Folder Renamed"));
        QVERIFY(QMetaObject::invokeMethod(editDialog, "openNew"));
        QTRY_VERIFY(editDialog->property("opened").toBool());
        nameField->setProperty("text", "keyboard folder renamed");
        QVERIFY(QMetaObject::invokeMethod(editDialog, "_save"));
        QVERIFY(!editDialog->property("_error").toString().isEmpty());   // duplicate name
        QVERIFY(QMetaObject::invokeMethod(editDialog, "close"));
        QTRY_VERIFY(!editDialog->property("visible").toBool());
        for (const auto& id : extraFolders) QVERIFY(folders->removeFolder(id));
        QVERIFY(folders->removeFolder(folderId));
        folders->setAllProjectsCollapsed(allCollapsed);
        folders->setUncategorizedCollapsed(uncategorizedCollapsed);
        c->setShowProjectSidebar(sidebarWasShown);
        window->resize(originalSize);
        window->hide();
        assertClean("folder keyboard navigation, activation and scrolling");
    }

    void test_01g_preferencesTabKeyboardNavigation()
    {
        clearWarnings();
        nav("selectSection", "settings");
        auto* window = qobject_cast<QQuickWindow*>(root);
        window->show();
        window->requestActivate();
        auto* settings = findVisualChild(window->contentItem(), "settingsPage");
        auto* tabs = findVisualChild(window->contentItem(), "settingsTabList");
        QVERIFY(settings); QVERIFY(tabs);
        QVariant selected;
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 0)));
        QVERIFY(selected.toBool());
        settings->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(tabs->property("currentIndex").toInt(), 1);
        QCOMPARE(settings->property("currentTab").toInt(), 0);
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(tabs->property("currentIndex").toInt(), 2);
        QTest::keyClick(window, Qt::Key_Return);
        QCOMPARE(settings->property("currentTab").toInt(), 2);
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(settings->property("currentTab").toInt(), 2);
        QCOMPARE(tabs->property("currentIndex").toInt(), 1);
        QTest::keyClick(window, Qt::Key_Enter);
        QCOMPARE(settings->property("currentTab").toInt(), 1);
        // The last built-in tab is Plugins (index 10); no plugin in this test
        // environment contributes Settings sections after it.
        for (int i = 0; i < 15; ++i) QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(tabs->property("currentIndex").toInt(), 10);
        QCOMPARE(settings->property("currentTab").toInt(), 1);
        QTest::keyClick(window, Qt::Key_Return);
        QCOMPARE(settings->property("currentTab").toInt(), 10);
        for (int i = 0; i < 15; ++i) QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(tabs->property("currentIndex").toInt(), 0);
        QTest::keyClick(window, Qt::Key_Return);
        QCOMPARE(settings->property("currentTab").toInt(), 0);
        // Arrows don't validate or save a draft; Enter retains the existing gate.
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 9)));
        QVERIFY(selected.toBool());
        auto* model = c->meetingTemplateEditorModel();
        QVERIFY(QMetaObject::invokeMethod(model, "beginNew"));
        model->setProperty("draftName", "Keyboard tab validation");
        tabs->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(settings->property("currentTab").toInt(), 9);
        QCOMPARE(tabs->property("currentIndex").toInt(), 8);
        auto* validation = root->findChild<QObject*>("meetingValidationDialog");
        QVERIFY(validation);
        QVERIFY(!validation->property("visible").toBool());
        QTest::keyClick(window, Qt::Key_Return);
        QCOMPARE(settings->property("currentTab").toInt(), 9);
        QTRY_VERIFY(validation->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(validation, "close"));
        QVERIFY(QMetaObject::invokeMethod(model, "deleteSelected"));
        QVERIFY(QMetaObject::invokeMethod(settings, "selectTab", Q_RETURN_ARG(QVariant, selected),
                                          Q_ARG(QVariant, 0)));
        QVERIFY(selected.toBool());
        nav("selectSection", "projects");
        auto* stack = findVisualChild(window->contentItem(), "contentStack");
        QTRY_VERIFY(!stack->property("busy").toBool());
        window->hide();
        assertClean("preferences tab highlighting, Enter and validation");
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
                                       "CompanyInput", "Primary ContactInput", "Status DateInput",
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
        QCOMPARE(tabList->property("count").toInt(), 11);
        QCOMPARE(pages->property("count").toInt(), 11);
        QVERIFY(findVisualChild(window->contentItem(), QStringLiteral("settingsTab_plugins")));
        assertClean("settings category tabs");
    }

    void test_02bb_preferencesSearchKeyboardSelection()
    {
        const QString originalId = c->managingCompanyId();
        const QString firstName = QStringLiteral("KeyboardChoice Alpha");
        const QString secondName = QStringLiteral("KeyboardChoice Beta");
        QStringList ids;
        for (const QString& name : {firstName, secondName}) {
            const int row = c->addCompany();
            QVERIFY(row >= 0);
            QVERIFY2(c->saveCompany(row, name), qPrintable(c->lastSaveError()));
            for (const QVariant& entry : c->companyList()) {
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
                                          Q_ARG(QVariant, 0)));
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
            const int row = c->companyRowForId(id);
            QVERIFY(row >= 0);
            QVERIFY2(c->deleteCompany(row), qPrintable(c->lastSaveError()));
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
        QTRY_VERIFY(reviewDialog->property("companyStage").toBool());
        QTRY_COMPARE(reviews->property("count").toInt(),1);
        auto *useLeftClient=findVisualChild(window->contentItem(),QStringLiteral("useLeftCompanyButton"));
        auto *useRightClient=findVisualChild(window->contentItem(),QStringLiteral("useRightCompanyButton"));
        auto *useBothClients=findVisualChild(window->contentItem(),QStringLiteral("useBothCompaniesButton"));
        QVERIFY(useLeftClient);QVERIFY(useLeftClient->isVisible());QVERIFY(useLeftClient->isEnabled());
        QVERIFY(useRightClient);QVERIFY(useRightClient->isVisible());QVERIFY(useRightClient->isEnabled());
        QVERIFY(useBothClients);QVERIFY(useBothClients->isVisible());QVERIFY(useBothClients->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(useRightClient,"clicked"));
        QTRY_VERIFY(!reviewDialog->property("companyStage").toBool());
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
                                          Q_ARG(QVariant, 3)));
        QVERIFY(selected.toBool());
        QTRY_COMPARE_WITH_TIMEOUT(pages->property("currentIndex").toInt(), 3, 1000);

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
        nav("selectSection", "contacts");
        const QString id = firstId(c->contactsModel());
        if (!id.isEmpty()) {
            nav("openContact", c->contactsRowForId(id), id);
            assertClean("openContact");
        }
    }

    void test_05_clientDetail()
    {
        clearWarnings();
        nav("selectSection", "companies");
        const QString id = firstId(c->companiesModel());
        if (!id.isEmpty()) {
            nav("openCompany", c->companyRowForId(id), id);
            assertClean("openCompany");
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

    // My Shortcuts' table: the URL cell's right-click menu offers Insert Field
    // with the preferences values plus the columns of the row's Entity, and the
    // Entity cell is a drop-down. Uses the plugin's real settings schema.
    void test_09b_schemaFormTableInsertField()
    {
        clearWarnings();
        QFile file(QStringLiteral(PN_TEST_FIXTURES) + QStringLiteral("/../../../plugins/myshortcuts/settings.schema.json"));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
        QQmlComponent component(engine);
        component.setData(R"(
            import QtQuick
            import ProjectNotesDesktop
            SchemaForm {
                id: form
                width: 900
                property string schemaJson: ""
                fields: JSON.parse(schemaJson).fields
                values: ({ shortcuts: [{ menu: "a", submenu: "", entity: "", url: "x" }] })
                // Every insertFields list under the form, as JSON, read inside QML where
                // a `var` property is plain JavaScript.
                function insertFieldLists() {
                    var out = []
                    function walk(item) {
                        if (item.spellCheckEnabled !== undefined)
                            out.push(item.insertFields)
                        for (var i = 0; i < item.children.length; ++i) walk(item.children[i])
                    }
                    walk(form)
                    return JSON.stringify(out)
                }
                function choiceOptions() {
                    var out = []
                    function walk(item) {
                        if (item.includeNone === true && item.options !== undefined && item.options.indexOf("projects") >= 0)
                            out.push(item)
                        for (var i = 0; i < item.children.length; ++i) walk(item.children[i])
                    }
                    walk(form)
                    return out.length ? out[0] : null
                }
            }
        )", QUrl());
        QScopedPointer<QObject> object(component.createWithInitialProperties(
            {{QStringLiteral("schemaJson"), QString::fromUtf8(file.readAll())}}));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* form = qobject_cast<QQuickItem*>(object.data());
        QVERIFY(form);
        QQuickWindow window;
        form->setParentItem(window.contentItem());
        window.resize(900, 600);
        window.show();

        auto insertFields = [&]() {
            QVariant json;
            QMetaObject::invokeMethod(form, "insertFieldLists", Q_RETURN_ARG(QVariant, json));
            QList<QStringList> found;
            for (const QJsonValue& list : QJsonDocument::fromJson(json.toString().toUtf8()).array()) {
                QStringList names;
                for (const QJsonValue& name : list.toArray())
                    names.append(name.toString());
                found.append(names);
            }
            return found;
        };
        QTRY_VERIFY_WITH_TIMEOUT(!insertFields().isEmpty(), 3000);
        // Four text cells in the row; only the URL cell lists any fields. A blank Entity
        // still offers the preferences values.
        auto longest = [&]() {
            int most = 0;
            for (const QStringList& list : insertFields())
                most = qMax(most, int(list.size()));
            return most;
        };
        QCOMPARE(longest(), 3);

        // Choose projects in the Entity drop-down.
        QObject* entityCombo = nullptr;
        {
            QVariant found;
            QMetaObject::invokeMethod(form, "choiceOptions", Q_RETURN_ARG(QVariant, found));
            entityCombo = found.value<QObject*>();
        }
        QVERIFY2(entityCombo, "Entity should be a drop-down");
        QVERIFY(QMetaObject::invokeMethod(entityCombo, "activated", Q_ARG(QString, QStringLiteral("projects"))));
        QTRY_VERIFY_WITH_TIMEOUT(longest() > 10, 3000);
        bool hasNumber = false;
        for (const QStringList& list : insertFields())
            hasNumber = hasNumber || list.contains(QStringLiteral("projects.project_number"));
        QVERIFY(hasNumber);
        assertClean("schema form table insert field");
    }

    // A plugin Settings section's table loads saved rows and saves edits. The
    // rows reach QML through PluginUiController::settingValues as a sequence
    // wrapper (Array.isArray false), which once left the table empty; an edit
    // reaches setSetting as a QJSValue, which QSettings once failed to store.
    void test_09c_pluginSettingsTableRoundTrip()
    {
        clearWarnings();
        PluginUiController* ui = PluginUiController::instance();
        QVERIFY(ui);
        const QString id = QStringLiteral("test.settingstable");
        const QVariantMap first {{QStringLiteral("menu"), QStringLiteral("a")}, {QStringLiteral("submenu"), QString()},
                                 {QStringLiteral("entity"), QStringLiteral("projects")}, {QStringLiteral("url"), QStringLiteral("x")}};
        QVariantMap second = first;
        second.insert(QStringLiteral("menu"), QStringLiteral("b"));
        ui->setSetting(id, QStringLiteral("shortcuts"), QVariantList{first, second});

        QFile file(QStringLiteral(PN_TEST_FIXTURES) + QStringLiteral("/../../../plugins/myshortcuts/settings.schema.json"));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
        const QVariantMap info {{QStringLiteral("pluginId"), id}, {QStringLiteral("id"), QStringLiteral("shortcuts")},
                                {QStringLiteral("title"), QStringLiteral("Shortcuts")},
                                {QStringLiteral("schema"), QJsonDocument::fromJson(file.readAll()).toVariant()}};

        QQmlComponent component(engine);
        component.setData(R"(
            import QtQuick
            import ProjectNotesDesktop
            PluginSettingsSection {
                id: section
                width: 900
                function find(item, test) {
                    if (test(item)) return item
                    for (var i = 0; i < item.children.length; ++i) {
                        var found = find(item.children[i], test)
                        if (found) return found
                    }
                    return null
                }
                function schemaForm() { return find(section, function (i) { return i._revision !== undefined }) }
                function storedRows() {
                    var v = schemaForm().value("shortcuts")
                    return Array.isArray(v) ? v.length : -1
                }
                // Rows on screen: each has one remove button (the button, not
                // the label inside it, has a clicked signal).
                function shownRows() {
                    var count = 0
                    function walk(item) {
                        if (item.text === "×" && typeof item.clicked === "function" && item.visible) ++count
                        for (var i = 0; i < item.children.length; ++i) walk(item.children[i])
                    }
                    walk(section)
                    return count
                }
                // What the Add Row button does.
                function addRow() {
                    var form = schemaForm()
                    form.setValue("shortcuts", form.value("shortcuts").concat([{ menu: "c", submenu: "", entity: "", url: "y" }]))
                }
            }
        )", QUrl());
        QScopedPointer<QObject> object(component.createWithInitialProperties({{QStringLiteral("info"), info}}));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* section = qobject_cast<QQuickItem*>(object.data());
        QVERIFY(section);
        QQuickWindow window;
        section->setParentItem(window.contentItem());
        window.resize(900, 600);
        window.show();

        auto call = [&](const char* name) {
            QVariant result;
            QMetaObject::invokeMethod(section, name, Q_RETURN_ARG(QVariant, result));
            return result.toInt();
        };
        QCOMPARE(call("storedRows"), 2);
        QTRY_COMPARE_WITH_TIMEOUT(call("shownRows"), 2, 3000);

        QVERIFY(QMetaObject::invokeMethod(section, "addRow"));
        QTRY_COMPARE_WITH_TIMEOUT(call("shownRows"), 3, 3000);
        const QVariant saved = ui->settingValues(id).value(QStringLiteral("shortcuts"));
        QCOMPARE(saved.typeId(), int(QMetaType::QVariantList));
        const QVariantList rows = saved.toList();
        QCOMPARE(rows.size(), 3);
        QCOMPARE(rows.at(0).toMap().value(QStringLiteral("menu")).toString(), QStringLiteral("a"));
        QCOMPARE(rows.at(2).toMap().value(QStringLiteral("menu")).toString(), QStringLiteral("c"));

        ui->setSetting(id, QStringLiteral("shortcuts"), QVariant());
        assertClean("plugin settings table round trip");
    }

    // Settings > Plugins backup: exportSettings writes a plugin's values
    // (tables included) to JSON and importSettings restores them. test_10
    // covers password fields and Python Settings Migrator files.
    void test_09d_pluginSettingsBackupRoundTrip()
    {
        clearWarnings();
        PluginUiController* ui = PluginUiController::instance();
        QVERIFY(ui);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString id = QStringLiteral("test.settingsbackup");
        const QVariantList rows {QVariantMap{{QStringLiteral("menu"), QStringLiteral("a")}},
                                 QVariantMap{{QStringLiteral("menu"), QStringLiteral("b")}}};
        ui->setSetting(id, QStringLiteral("rows"), rows);
        ui->setSetting(id, QStringLiteral("folder"), QStringLiteral("Plans"));

        const QUrl backup = QUrl::fromLocalFile(dir.filePath(QStringLiteral("backup.json")));
        QCOMPARE(ui->exportSettings(backup, {id}), QString());
        QFile file(backup.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        QCOMPARE(root.value(QStringLiteral("format")).toString(), QStringLiteral("projectnotes-plugin-settings"));
        QCOMPARE(root.value(QStringLiteral("plugins")).toObject().value(id).toObject()
                     .value(QStringLiteral("rows")).toArray().size(), 2);
        QVERIFY(!ui->exportSettings(QUrl::fromLocalFile(dir.filePath(QStringLiteral("none.json"))),
                                    {QStringLiteral("test.nosettings")}).isEmpty());

        ui->setSetting(id, QStringLiteral("rows"), QVariant());
        ui->setSetting(id, QStringLiteral("folder"), QStringLiteral("Changed"));
        const int revision = ui->settingsRevision();
        QVariantMap result = ui->importSettings(backup);
        QVERIFY2(result.value(QStringLiteral("ok")).toBool(), qPrintable(result.value(QStringLiteral("error")).toString()));
        QCOMPARE(result.value(QStringLiteral("count")).toInt(), 2);
        QCOMPARE(ui->settingsRevision(), revision + 1);
        QVariantMap values = ui->settingValues(id);
        QCOMPARE(values.value(QStringLiteral("folder")).toString(), QStringLiteral("Plans"));
        const QVariantList restored = values.value(QStringLiteral("rows")).toList();
        QCOMPARE(restored.size(), 2);
        QCOMPARE(restored.at(1).toMap().value(QStringLiteral("menu")).toString(), QStringLiteral("b"));

        QFile bad(dir.filePath(QStringLiteral("bad.json")));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("not json");
        bad.close();
        QVERIFY(!ui->importSettings(QUrl::fromLocalFile(bad.fileName())).value(QStringLiteral("ok")).toBool());

        ui->setSetting(id, QStringLiteral("rows"), QVariant());
        ui->setSetting(id, QStringLiteral("folder"), QVariant());
        assertClean("plugin settings backup round trip");
    }

    // File > Export / Import Settings: application settings and plugin settings
    // in one file. Secrets and window layout stay out of the file.
    void test_09e_allSettingsBackupRoundTrip()
    {
        clearWarnings();
        PluginUiController* ui = PluginUiController::instance();
        QVERIFY(ui);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString org = DesktopAppController::localSettingsOrganization();
        const QString id = QStringLiteral("test.allsettings");
        {
            QSettings app(org, QStringLiteral("AppSettings"));
            app.setValue(QStringLiteral("Email/v1/SenderAddress"), QStringLiteral("me@example.com"));
            app.setValue(QStringLiteral("Sync/Password"), QStringLiteral("hunter2"));
            app.setValue(QStringLiteral("Sync/EncryptionPhrase"), QStringLiteral("phrase"));
            app.setValue(QStringLiteral("UI:QMLMainWindow:Width"), 1234);
        }
        ui->setSetting(id, QStringLiteral("folder"), QStringLiteral("Plans"));

        const QUrl backup = QUrl::fromLocalFile(dir.filePath(QStringLiteral("all.json")));
        QCOMPARE(ui->exportAllSettings(backup), QString());
        QFile file(backup.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        QCOMPARE(root.value(QStringLiteral("format")).toString(), QStringLiteral("projectnotes-settings"));
        const QJsonObject appJson = root.value(QStringLiteral("app")).toObject();
        QCOMPARE(appJson.value(QStringLiteral("Email/v1/SenderAddress")).toString(), QStringLiteral("me@example.com"));
        QVERIFY(!appJson.contains(QStringLiteral("Sync/Password")));
        QVERIFY(!appJson.contains(QStringLiteral("Sync/EncryptionPhrase")));
        QVERIFY(!appJson.contains(QStringLiteral("UI:QMLMainWindow:Width")));
        QCOMPARE(root.value(QStringLiteral("plugins")).toObject().value(id).toObject()
                     .value(QStringLiteral("folder")).toString(), QStringLiteral("Plans"));

        {
            QSettings app(org, QStringLiteral("AppSettings"));
            app.setValue(QStringLiteral("Email/v1/SenderAddress"), QStringLiteral("changed@example.com"));
        }
        ui->setSetting(id, QStringLiteral("folder"), QStringLiteral("Changed"));
        const QVariantMap result = ui->importAllSettings(backup);
        QVERIFY2(result.value(QStringLiteral("ok")).toBool(), qPrintable(result.value(QStringLiteral("error")).toString()));
        QVERIFY(result.value(QStringLiteral("appCount")).toInt() > 0);
        QCOMPARE(ui->settingValues(id).value(QStringLiteral("folder")).toString(), QStringLiteral("Plans"));
        {
            QSettings app(org, QStringLiteral("AppSettings"));
            QCOMPARE(app.value(QStringLiteral("Email/v1/SenderAddress")).toString(), QStringLiteral("me@example.com"));
            QCOMPARE(app.value(QStringLiteral("Sync/Password")).toString(), QStringLiteral("hunter2"));
            app.remove(QStringLiteral("Email/v1/SenderAddress"));
            app.remove(QStringLiteral("Sync/Password"));
            app.remove(QStringLiteral("Sync/EncryptionPhrase"));
            app.remove(QStringLiteral("UI:QMLMainWindow:Width"));
        }

        // The plugin-only import reads the plugin part of a full backup.
        ui->setSetting(id, QStringLiteral("folder"), QStringLiteral("Changed"));
        QVERIFY(ui->importSettings(backup).value(QStringLiteral("ok")).toBool());
        QCOMPARE(ui->settingValues(id).value(QStringLiteral("folder")).toString(), QStringLiteral("Plans"));

        QFile other(dir.filePath(QStringLiteral("other.json")));
        QVERIFY(other.open(QIODevice::WriteOnly));
        other.write("{\"group\": {\"key\": 1}}");
        other.close();
        QVERIFY(!ui->importAllSettings(QUrl::fromLocalFile(other.fileName())).value(QStringLiteral("ok")).toBool());

        ui->setSetting(id, QStringLiteral("folder"), QVariant());
        assertClean("all settings backup round trip");
    }

    void cleanupTestCase()
    {
        delete engine;   // tear the window down before the app object goes away
        engine = nullptr;
    }

    // A JavaScript plugin end to end in the real UI: installing into the user
    // plugin folder, approval, its Settings section, a schema form through
    // PluginUiHost, and a plugin QML dialog importing ProjectNotes.PluginUI
    // that calls back into the plugin. Fails on any QML warning.
    void test_10_jsPluginUi()
    {
        clearWarnings();
        PN::Plugins::PluginRegistry* registry = c->pluginRegistry();
        QVERIFY(registry);
        PluginUiController* ui = PluginUiController::instance();
        const QString id = QStringLiteral("test.uitest");

        // Install from the fixture folder, then approve its permissions.
        QString installedId;
        QString error;
        QVERIFY2(registry->install(QStringLiteral(PN_TEST_FIXTURES) + QStringLiteral("/plugins/uitest"),
                                   &installedId, &error), qPrintable(error));
        QCOMPARE(installedId, id);
        QVERIFY(registry->plugin(id).needsApproval);
        QCOMPARE(ui->approve(id), QString());
        QTRY_VERIFY_WITH_TIMEOUT(registry->isRunning(id), 10000);

        // Settings: the Plugins pane and the plugin's own section tab.
        nav("selectSection", "settings");
        QQuickWindow* window = qobject_cast<QQuickWindow*>(root);
        QVERIFY(window);
        QTRY_VERIFY(findVisualChild(window->contentItem(), QStringLiteral("settingsTab_plugin_test.uitest_general")));
        QQuickItem* tabList = findVisualChild(window->contentItem(), QStringLiteral("settingsTabList"));
        QTRY_COMPARE(tabList->property("count").toInt(), 12);
        ui->setSetting(id, QStringLiteral("color"), QStringLiteral("Blue"));
        QCOMPARE(ui->settingValues(id).value(QStringLiteral("color")).toString(), QStringLiteral("Blue"));
        nav("selectSection", "projects");

        // Settings backup: password fields stay out of the file, and a Python
        // Settings Migrator file reaches this plugin through legacySettings.
        {
            QTemporaryDir dir;
            QVERIFY(dir.isValid());
            // A password field goes to the plugin's credential-store namespace,
            // never to the plugin settings; a blank one keeps what is stored.
            QHash<QString, QString> secrets;
            PluginUiController::setSecretWriter([&secrets](const QString& plugin, const QString& key, const QString& secret, QString*) {
                secrets.insert(plugin + QLatin1Char('/') + key, secret);
                return true;
            });
            auto restoreWriter = qScopeGuard([] {
                PluginUiController::setSecretWriter([](const QString&, const QString&, const QString&, QString*) { return true; });
            });
            ui->setSetting(id, QStringLiteral("token"), QStringLiteral("hunter2"));
            QCOMPARE(secrets.value(id + QStringLiteral("/token")), QStringLiteral("hunter2"));
            QVERIFY(!ui->settingValues(id).contains(QStringLiteral("token")));
            ui->setSetting(id, QStringLiteral("token"), QString());
            QCOMPARE(secrets.value(id + QStringLiteral("/token")), QStringLiteral("hunter2"));
            bool listed = false;
            for (const QVariant& candidate : ui->settingsBackupCandidates())
                listed = listed || candidate.toMap().value(QStringLiteral("id")).toString() == id;
            QVERIFY(listed);
            const QUrl backup = QUrl::fromLocalFile(dir.filePath(QStringLiteral("backup.json")));
            QCOMPARE(ui->exportSettings(backup, {id}), QString());
            QFile file(backup.toLocalFile());
            QVERIFY(file.open(QIODevice::ReadOnly));
            const QJsonObject saved = QJsonDocument::fromJson(file.readAll()).object()
                                          .value(QStringLiteral("plugins")).toObject().value(id).toObject();
            QCOMPARE(saved.value(QStringLiteral("color")).toString(), QStringLiteral("Blue"));
            QVERIFY(!saved.contains(QStringLiteral("token")));
            ui->setSetting(id, QStringLiteral("token"), QVariant());

            QFile legacy(dir.filePath(QStringLiteral("legacy.json")));
            QVERIFY(legacy.open(QIODevice::WriteOnly));
            legacy.write(R"({ "UI Test Legacy": { "Folder": "from python" } })");
            legacy.close();
            const QVariantMap result = ui->importSettings(QUrl::fromLocalFile(legacy.fileName()));
            QVERIFY2(result.value(QStringLiteral("ok")).toBool(), qPrintable(result.value(QStringLiteral("error")).toString()));
            QCOMPARE(result.value(QStringLiteral("plugins")).toStringList(), QStringList{QStringLiteral("UI Test Plugin")});
            QCOMPARE(ui->settingValues(id).value(QStringLiteral("OldFolder")).toString(), QStringLiteral("from python"));
            ui->setSetting(id, QStringLiteral("OldFolder"), QVariant());
        }

        // A schema form: answer it as the OK button would.
        registry->runMenu(id, QStringLiteral("form"), {}, {});
        QObject* form = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT((form = root->findChild<QObject*>(QStringLiteral("pluginRequest_form"))) != nullptr, 10000);
        QTRY_VERIFY(form->property("visible").toBool());
        ui->respond(form->property("requestId").toInt(),
                    QVariantMap{{QStringLiteral("name"), QStringLiteral("Zed")}, {QStringLiteral("kind"), QStringLiteral("B")}});
        QTRY_COMPARE_WITH_TIMEOUT(ui->settingValues(id).value(QStringLiteral("formResult")).toMap()
                                      .value(QStringLiteral("name")).toString(), QStringLiteral("Zed"), 10000);
        QTRY_VERIFY(!root->findChild<QObject*>(QStringLiteral("pluginRequest_form")));

        // A plugin QML dialog that invokes a plugin handler and accepts itself.
        registry->runMenu(id, QStringLiteral("dialog"), {}, {});
        QTRY_VERIFY2_WITH_TIMEOUT(ui->settingValues(id).value(QStringLiteral("dialogResult")).toString() == QLatin1String("HI back"),
                                  qPrintable(m_warnings.join(QStringLiteral(" | ")) + QStringLiteral(" || ") + registry->plugin(id).lastError),
                                  10000);
        QTRY_VERIFY(!root->findChild<QObject*>(QStringLiteral("pluginRequest_dialog")));

        QVERIFY2(registry->plugin(id).lastError.isEmpty(), qPrintable(registry->plugin(id).lastError));
        QVERIFY2(registry->uninstall(id, &error), qPrintable(error));
        assertClean("JavaScript plugin UI");
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

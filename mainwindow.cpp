// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ui_mainwindow.h"

#include "tableview.h"
#include "allitemspage.h"
#include "projectslistmodel.h"
#include "databaseobjects.h"
#include "aboutdialog.h"
#include "plaintextedit.h"
#include "textedit.h"
#include "combobox.h"

#include <QStringListModel>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextEdit>
#include <QTextList>
#include <QColorDialog>
#include <QClipboard>
#include <QMimeType>
#include <QMimeData>
#include <QActionGroup>
#include <QDesktopServices>
#include <QLineEdit>
#include "mainwindow.h"
#include "cloudsyncsettingsdialog.h"
#include "appsettings.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

PluginManager* MainWindow::m_pluginManager = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QLoggerManager *logmanager = QLoggerManager::getInstance();

    QString logloc = LogViewer::getLogFileLocation();
    #ifdef QT_DEBUG
        logmanager->addDestination("debugging.log", DEBUGLOG, LogLevel::Debug, logloc, LogMode::OnlyFile);
    #endif

    logmanager->addDestination("error.log", ERRORLOG, LogLevel::Error, logloc, LogMode::OnlyFile);
    logmanager->addDestination("console.log", CONSOLELOG, LogLevel::Info, logloc, LogMode::OnlyFile);

    logmanager->resume();

    // add special formatting button for html editor
    setupTextActions();

    m_preferencesDialog = new PreferencesDialog(this);
    m_findReplaceDialog = new FindReplaceDialog(this);

    // view state
    m_pageHistory.clear();
    m_navigationLocation = -1;
    m_forwardBackHistory.clear();

    int sz = global_Settings.getStoredInt("DefaultFontSize");

    if (sz == -1)
        sz = 10;

    QFont af = QApplication::font();
    af.setPointSize(sz);
    QApplication::setFont(af);

    connect(ui->tableViewProjects, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ProjectDetails_triggered(QVariant)));
    connect(ui->tableViewTrackerItems, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ItemDetails_triggered(QVariant)));
    connect(ui->tableViewAllItems, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ItemDetails_triggered(QVariant)));
    connect(ui->tableViewActionItems, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ItemDetails_triggered(QVariant)));
    connect(ui->tableViewProjectNotes, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ProjectNote_triggered(QVariant)));
    connect(ui->tableViewSearchResults, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_SearchResults_triggered(QVariant)));
    connect(ui->tableViewTeam, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpenTeamMember_triggered(QVariant)));
    connect(ui->tableViewAtendees, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpenTeamMember_triggered(QVariant)));
    connect(ui->tableViewLocations, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpenLocation_triggered(QVariant)));

    connect(ui->textEditNotes, &QTextEdit::currentCharFormatChanged, this, &MainWindow::currentCharFormatChanged);
    connect(ui->textEditNotes, &QTextEdit::cursorPositionChanged, this, &MainWindow::cursorPositionChanged);

    connect(dynamic_cast<QApplication*>(QApplication::instance()), &QApplication::focusChanged, this, &MainWindow::on_focusChanged);

    m_syncProgressBar = new QProgressBar(this);
    m_syncProgressBar->setRange(0, 100);
    m_syncProgressBar->setFixedWidth(200);
    m_syncProgressBar->setTextVisible(false);
    m_syncProgressBar->hide();
    ui->statusbar->addPermanentWidget(m_syncProgressBar);

    m_syncNetworkErrorLabel = new QLabel(this);
    m_syncNetworkErrorLabel->setPixmap(
        style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(16, 16));
    m_syncNetworkErrorLabel->setToolTip(tr("Unable to connect to sync host"));
    m_syncNetworkErrorLabel->hide();
    ui->statusbar->addPermanentWidget(m_syncNetworkErrorLabel);

    // Monitor OS-level network reachability so the disconnect icon appears
    // immediately when the network goes down — without waiting for the next
    // sync cycle to run and fail.
    if (QNetworkInformation::loadDefaultBackend() && QNetworkInformation::instance()) {
        connect(QNetworkInformation::instance(),
                &QNetworkInformation::reachabilityChanged,
                this, &MainWindow::onNetworkReachabilityChanged,
                Qt::QueuedConnection);
    }

    m_pluginManager = new PluginManager(this);

    connect(m_pluginManager, &PluginManager::pluginLoaded, this, &MainWindow::onPluginLoaded);
    connect(m_pluginManager, &PluginManager::pluginUnLoaded, this, &MainWindow::onPluginUnLoaded);
    connect(m_pluginManager, &PluginManager::pluginRefreshRequest, this, &MainWindow::onRefreshRequested);

    // Non-threaded plugins fire pluginLoaded synchronously during new PluginManager(this),
    // before the connections above are established, so onPluginLoaded is never called for them.
    // Build the plugin menu now so their menu items appear regardless of database state.
    buildPluginMenu(nullptr);

    // Always open the database from the standard app data location, creating if needed.
    // When running under a developer profile, files go in a profile-named subfolder so
    // development data (database + logs) is fully isolated from production.
    const QString dbfile = AppSettings::dataLocation() + "/ProjectNotes.db";

    if (!QFile::exists(dbfile)) {
        QDir().mkpath(QFileInfo(dbfile).absolutePath());
        global_DBObjects.createDatabase(dbfile);
    }

    openDatabase(dbfile);

    setButtonAndMenuStates();
}

void MainWindow::addMenuItem(QMenu* menu, const QString& submenu, const QString& menutitle, QAction* action, int section)
{
    if (submenu.isEmpty())
    {
        QAction* nextaction = nullptr;
        int pastseparator = 0;

        for (QAction* existing : menu->actions())
        {
            QString itemtitle = existing->text().replace("&","");

            if (pastseparator >= section && itemtitle.compare(menutitle, Qt::CaseInsensitive) > 0)
            {
                nextaction = existing;
                break;
            }

            if (existing->isSeparator())
                pastseparator++;
        }

        menu->insertAction(nextaction, action);
    }
    else
    {
        // find the submenu if it exists
        QMenu* subMenuPtr = nullptr;

        int pastseparator = 0;

        for (QAction* existing : menu->actions())
        {
            QString itemtitle = existing->text().replace("&","");

            if (pastseparator >= section && itemtitle.compare(submenu, Qt::CaseInsensitive) == 0 && existing->menu() != nullptr)
            {
                addMenuItem(existing->menu(), QString(), menutitle, action, 0);
                return;
            }

            if (existing->isSeparator())
                pastseparator++;
        }

        // if it didn't exist create it sorted
        if (!subMenuPtr)
        {
            pastseparator = 0;
            QAction* nextaction = nullptr;

            for (QAction* existing : menu->actions())
            {
                QString itemtitle = existing->text().replace("&","");

                if (pastseparator >= section && itemtitle.compare(submenu, Qt::CaseInsensitive) > 0)
                {
                    nextaction = existing;
                    break;
                }

                if (existing->isSeparator())
                    pastseparator++;
            }

            subMenuPtr = new QMenu(submenu);
            QAction* subMenuAction = menu->insertMenu(nextaction, subMenuPtr);
            // On macOS, Qt auto-assigns PreferencesRole to actions whose text matches
            // "Settings" or "Preferences", moving them to the Application menu.
            // Override this so plugin submenus stay in the Plugins menu.
            if (subMenuAction)
                subMenuAction->setMenuRole(QAction::NoRole);
            addMenuItem(subMenuPtr, QString(), menutitle, action, 0);
        }
    }
}


void MainWindow::buildPluginMenu(BasePage* currentPage)
{
    // clear any other plugin items
    QMenu *menu = ui->menuPlugins;
    int itemCount = menu->actions().count() - 1;

    for (int i = itemCount; i > 0; i--)
    {
        QAction *action = menu->actions().at(i);
        menu->removeAction(action);
        delete action;
    }

    menu->addSeparator();

    // add globally available plugins
    for ( Plugin* p : m_pluginManager->plugins())
    {
        for ( PluginMenu m : p->pythonplugin().menus())
        {
            if (m.dataexport().isEmpty()) // don't include any right-click data menus
            {
                QAction* act = new QAction(QIcon(":/icons/add-on.png"), m.menutitle(), this);
                connect(act, &QAction::triggered, this,[p, m, this](){slotPluginMenu(p, m.functionname(), m.parameter());});
                addMenuItem(ui->menuPlugins, m.submenu(), m.menutitle(), act, 1);
            }
        }
    }

    // if we have a current page add it's items to the plugin menu
    if (currentPage)
        currentPage->buildPluginMenu(m_pluginManager, menu);
}

void MainWindow::slotPluginMenu(Plugin* plugin, const QString& functionname, const QString& parameter)
{
    plugin->callMethod(functionname, parameter);
}

MainWindow::~MainWindow()
{
    disconnect(ui->tableViewProjects, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ProjectDetails_triggered(QVariant)));
    disconnect(ui->tableViewTrackerItems, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ItemDetails_triggered(QVariant)));
    disconnect(ui->tableViewActionItems, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ItemDetails_triggered(QVariant)));
    disconnect(ui->tableViewProjectNotes, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_ProjectNote_triggered(QVariant)));
    disconnect(ui->tableViewSearchResults, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpen_SearchResults_triggered(QVariant)));
    disconnect(ui->tableViewTeam, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpenTeamMember_triggered(QVariant)));
    disconnect(ui->tableViewAtendees, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpenTeamMember_triggered(QVariant)));
    disconnect(ui->tableViewLocations, SIGNAL(signalOpenRecordWindow(QVariant)), this, SLOT(slotOpenLocation_triggered(QVariant)));

    disconnect(ui->textEditNotes, &QTextEdit::currentCharFormatChanged, this, &MainWindow::currentCharFormatChanged);
    disconnect(ui->textEditNotes, &QTextEdit::cursorPositionChanged, this, &MainWindow::cursorPositionChanged);

    disconnect(m_pluginManager, &PluginManager::pluginLoaded, this, &MainWindow::onPluginLoaded);
    disconnect(m_pluginManager, &PluginManager::pluginUnLoaded, this, &MainWindow::onPluginUnLoaded);
    disconnect(m_pluginManager, &PluginManager::pluginRefreshRequest, this, &MainWindow::onRefreshRequested);


    // need to save the screen layout befor the model is removed from the view
    // The destructor of PNTableview does not save the state
    ui->tableViewProjects->setModel(nullptr);
    ui->tableViewClients->setModel(nullptr);
    ui->tableViewPeople->setModel(nullptr);
    ui->tableViewStatusReportItems->setModel(nullptr);
    ui->tableViewTeam->setModel(nullptr);
    ui->tableViewTrackerItems->setModel(nullptr);
    ui->tableViewAllItems->setModel(nullptr);
    ui->tableViewAtendees->setModel(nullptr);
    ui->tableViewProjectNotes->setModel(nullptr);
    ui->tableViewLocations->setModel(nullptr);
    ui->tableViewComments->setModel(nullptr);

    while (m_pageHistory.count())
    {
        HistoryNode* hn = m_pageHistory.pop();
        delete hn;
    }

    while (m_forwardBackHistory.count())
    {
        HistoryNode* hn = m_forwardBackHistory.pop();
        delete hn;
    }

    if (global_DBObjects.isOpen())
        CloseDatabase();

    if (m_waitForThreadsTimer)
    {
        disconnect(m_waitForThreadsTimer, &QTimer::timeout, this, &MainWindow::onTimerWaitForThreads);

        delete m_waitForThreadsTimer;
    }

    delete m_preferencesDialog;
    delete m_findReplaceDialog;
    delete m_pluginManager;
    delete ui;

    ui = nullptr;
}

void MainWindow::on_focusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old);
    Q_UNUSED(now);

    setButtonAndMenuStates();
}

void MainWindow::setButtonAndMenuStates()
{
    if (!ui)
        return;

    ui->actionStatus_Bar->setChecked(ui->statusbar->isVisible());

    bool dbopen = global_DBObjects.isOpen();

    ui->stackedWidget->setVisible(dbopen);

    ui->actionSearch->setEnabled(dbopen);
    ui->actionSync_All->setEnabled(m_syncApi && m_syncApi->isInitialized());

    if (m_syncProgressBar && (!m_syncApi || !m_syncApi->isInitialized()))
        m_syncProgressBar->hide();


    TableView* curview = nullptr;

    if (navigateCurrentPage())
    {
        // if we have current page, call it's button and menu state function
        navigateCurrentPage()->setButtonAndMenuStates();

        curview = navigateCurrentPage()->getCurrentView();
    }

    if (curview && curview->selectionModel())
    {
        bool issearch = (curview->objectName().compare("tableViewSearchResults") == 0);
        bool sel = curview->selectionModel()->hasSelection();

        // can only choose export when something is selected
        ui->actionXML_Export->setEnabled(dbopen && sel && !issearch);
        ui->actionFilter->setEnabled(dbopen);
    }
    else
    {
        ui->actionXML_Export->setEnabled(false);
        ui->actionFilter->setEnabled(false);
    }

    bool hascurview = (curview != nullptr);

    ui->actionXML_Import->setEnabled(dbopen);

    ui->actionInternal_Items->setEnabled(dbopen);
    ui->actionAll_Tracker_Action_Items->setEnabled(dbopen);

    ui->actionProjects->setEnabled(dbopen);
    ui->actionClosed_Projects->setEnabled(dbopen);

    bool pageAllowsNewDelete = !navigateCurrentPage() || navigateCurrentPage()->allowNewDelete();

    ui->actionOpen_Item->setEnabled(hascurview);
    ui->actionNew_Item->setEnabled(hascurview && pageAllowsNewDelete);

    // Don't allow copying team members or meeting attendees
    bool canCopy = hascurview && pageAllowsNewDelete;
    if (curview && canCopy)
    {
        QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(curview->model());
        if (sortmodel)
        {
            SqlQueryModel* sourcemodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());
            if (sourcemodel && (sourcemodel->objectName() == "ProjectTeamMembersModel" || sourcemodel->objectName() == "MeetingAttendeesModel"))
                canCopy = false;
        }
    }
    ui->actionCopy_Item->setEnabled(canCopy);

    ui->actionDelete_Item->setEnabled(hascurview && pageAllowsNewDelete);
    ui->actionEdit_Items->setEnabled(hascurview);

    ui->actionBack->setEnabled(!navigateAtStart());
    ui->actionForward->setEnabled(!navigateAtEnd());

    ui->actionClients->setEnabled(dbopen);
    ui->actionPeople->setEnabled(dbopen);
    ui->actionMasterItemList->setEnabled(dbopen);


    //plugin menu
    if (m_logviewDialog)
    {
        if (m_logviewDialog->isVisible())
            ui->actionView_LogView->setChecked(true);
        else
            ui->actionView_LogView->setChecked(false);
    }
    else
        ui->actionView_LogView->setChecked(false);

    if (dbopen)
    {
        ui->actionClosed_Projects->setChecked(global_DBObjects.getShowClosedProjects());
        ui->actionInternal_Items->setChecked(global_DBObjects.getShowInternalItems());

        if (global_DBObjects.getShowInternalItems())
        {
            ui->tableViewTrackerItems->setColumnHidden(15, false);
            ui->tableViewTrackerItems->resizeColumnToContents(15);
            ui->tableViewAllItems->setColumnHidden(15, false);
            ui->tableViewAllItems->resizeColumnToContents(15);

            ui->tableViewProjects->setColumnHidden(6, false);
            ui->tableViewProjects->setColumnHidden(7, false);
            ui->tableViewProjects->setColumnHidden(8, false);
            ui->tableViewProjects->setColumnHidden(9, false);
            ui->tableViewProjects->setColumnHidden(10, false);
            ui->tableViewProjects->setColumnHidden(15, false);
            ui->tableViewProjects->setColumnHidden(16, false);
            ui->tableViewProjects->setColumnHidden(17, false);
            ui->tableViewProjects->setColumnHidden(18, false);
            ui->tableViewProjects->setColumnHidden(20, false);

            ui->tableViewProjects->resizeColumnToContents(6);
            ui->tableViewProjects->resizeColumnToContents(7);
            ui->tableViewProjects->resizeColumnToContents(8);
            ui->tableViewProjects->resizeColumnToContents(9);
            ui->tableViewProjects->resizeColumnToContents(10);
            ui->tableViewProjects->resizeColumnToContents(15);
            ui->tableViewProjects->resizeColumnToContents(16);
            ui->tableViewProjects->resizeColumnToContents(17);
            ui->tableViewProjects->resizeColumnToContents(18);
            ui->tableViewProjects->resizeColumnToContents(20);
        }
        else
        {
            ui->tableViewTrackerItems->setColumnHidden(15, true);
            ui->tableViewAllItems->setColumnHidden(15, true);

            ui->tableViewProjects->setColumnHidden(6, true);
            ui->tableViewProjects->setColumnHidden(7, true);
            ui->tableViewProjects->setColumnHidden(8, true);
            ui->tableViewProjects->setColumnHidden(9, true);
            ui->tableViewProjects->setColumnHidden(10, true);
            ui->tableViewProjects->setColumnHidden(15, true);
            ui->tableViewProjects->setColumnHidden(16, true);
            ui->tableViewProjects->setColumnHidden(17, true);
            ui->tableViewProjects->setColumnHidden(18, true);
            ui->tableViewProjects->setColumnHidden(20, true);
        }

        ui->tabWidgetProject->setTabText(2, "Tracker");

        ui->tableViewTrackerItems->setColumnHidden(0, true);
        ui->tableViewTrackerItems->setColumnHidden(14, true);
        ui->tableViewTrackerItems->setColumnHidden(17, true);
        ui->tableViewTrackerItems->setColumnHidden(18, true);

        ui->tableViewAllItems->setColumnHidden(0, true);
        ui->tableViewAllItems->setColumnHidden(18, true);

        QWidget* fw = this->focusWidget();
        const QLatin1StringView fwClass(fw ? fw->metaObject()->className() : "");

        // determind if we can format text
        bool can_format_text =
            (fw != nullptr) &&
            (fwClass == "TextEdit");

        // determine if we can text edit
        bool can_text_edit =
                (fw != nullptr) && (
                can_format_text ||
                fwClass == "QLineEdit" ||
                fwClass == "QExpandingLineEdit" ||
                fwClass == "ComboBox" ||
                fwClass == "PlainTextEdit");

        // determine if we can find text
        bool can_find_edit =
                (fw != nullptr) && (
                can_format_text ||
                fwClass == "QLineEdit" ||
                fwClass == "TextEdit" ||
                fwClass == "PlainTextEdit");

        // can't edit combo boxes not set to editable
        if (can_text_edit && fwClass == "ComboBox")
        {
            if (!(dynamic_cast<ComboBox*>(fw))->isEditable())
                can_text_edit = false;
        }

        // file menu items
        ui->actionSearch->setEnabled(true);
        ui->actionXML_Import->setEnabled(true);
        ui->actionPreferences->setEnabled(true);

        // edit menu items
        ui->actionFind->setEnabled(can_find_edit);
        ui->actionSpell_Check->setEnabled(can_text_edit);
        ui->actionUndo->setEnabled(can_text_edit);
        ui->actionRedo->setEnabled(can_text_edit);
        ui->actionDelete->setEnabled(can_text_edit);
        ui->actionCut->setEnabled(can_text_edit);
        ui->actionCopy->setEnabled(can_text_edit);
        ui->actionPaste->setEnabled(can_text_edit);
        ui->actionSelect_All->setEnabled(can_text_edit);

        // check if form has table view available to use
        // Note Page Note Tab does not have a table view
         if ( ui->tabWidgetNotes->currentIndex() == 0 && ui->stackedWidget->currentIndex() == 1 )
        { 
            ui->actionDelete_Item->setEnabled(false);
            ui->actionOpen_Item->setEnabled(false);
            ui->actionCopy_Item->setEnabled(false);
            ui->actionNew_Item->setEnabled(false);

            ui->toolBarFormat->setVisible(true); // text format bar
            ui->toolBarEdit->setVisible(true); // text edit bar
        }
        else
        {
            ui->toolBarFormat->setVisible(false); // text format bar
            ui->toolBarEdit->setVisible(false); // text edit bar

            if (curview && curview->selectionModel()->hasSelection())
            {
                ui->actionDelete_Item->setEnabled(hascurview && pageAllowsNewDelete);
                ui->actionOpen_Item->setEnabled(hascurview);

                // Don't allow copying team members or meeting attendees
                bool canCopy = hascurview && pageAllowsNewDelete;
                QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(curview->model());
                if (sortmodel)
                {
                    SqlQueryModel* sourcemodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());
                    if (sourcemodel && (sourcemodel->objectName() == "ProjectTeamMembersModel" || sourcemodel->objectName() == "MeetingAttendeesModel"))
                        canCopy = false;
                }
                ui->actionCopy_Item->setEnabled(canCopy);
            }
            else
            {
                ui->actionDelete_Item->setEnabled(false);
                ui->actionOpen_Item->setEnabled(false);
                ui->actionCopy_Item->setEnabled(false);
            }

            ui->actionNew_Item->setEnabled(hascurview && pageAllowsNewDelete);
        }

        // format menu items
        if ( can_format_text )
            ui->menuFormat->setEnabled(true);
        else
            ui->menuFormat->setEnabled(false);

        // view items
        ui->menuView->setEnabled(true);

        // filter tracker items
        ui->actionResolved_Tracker_Action_Items->setChecked(!global_DBObjects.getShowResolvedTrackerItems());
        ui->actionResolved_Tracker_Action_Items->setEnabled(true);

        // clear out page history for a rebuild
        ui->menuHistory->clear();

        int i = m_pageHistory.count();
        while(i)
        {
            i--;
            HistoryNode* hn = m_pageHistory.at(i);
            QString title = QString("%1 - %2").arg(hn->m_pagetitle, hn->m_timestamp.toString("MM/dd/yy hh:mm"));
            QAction* ha = ui->menuHistory->addAction(title, [hn, this](){navigateToPage(hn->m_page, hn->m_recordId);});
            ha->setPriority(QAction::LowPriority);
        }
    }
    else
    {
        // file menu items
        ui->actionSearch->setEnabled(false);
        ui->actionXML_Import->setEnabled(false);
        ui->actionPreferences->setEnabled(false);

        // edit menu items
        ui->actionFind->setEnabled(false);
        ui->actionSpell_Check->setEnabled(false);
        ui->actionUndo->setEnabled(false);
        ui->actionRedo->setEnabled(false);
        ui->actionDelete->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionCopy->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionSelect_All->setEnabled(false);

        ui->actionDelete_Item->setEnabled(false);
        ui->actionOpen_Item->setEnabled(false);
        ui->actionCopy_Item->setEnabled(false);
        ui->actionNew_Item->setEnabled(false);

        // format menu items
        ui->menuFormat->setEnabled(false);

        // view items
        ui->menuView->setEnabled(false);

        // text format bar
        ui->toolBarFormat->setVisible(false);

        // edit tool bar
        ui->toolBarEdit->setVisible(false);

        // filter tracker items
        ui->actionResolved_Tracker_Action_Items->setChecked(false);
        ui->actionResolved_Tracker_Action_Items->setEnabled(false);

        // clear out page history
        ui->menuHistory->clear();
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionSync_All_triggered()
{
    ui->statusbar->showMessage(tr("Re-syncing all records..."));
    if (m_syncApi)
        m_syncApi->syncAll();
}

void MainWindow::on_actionOpen_Database_triggered()
{
    CloudSyncSettingsDialog dlg(this);

    // Pre-populate from saved settings
    dlg.setSyncEnabled(global_Settings.getSyncEnabled());
    dlg.setSyncHostType(global_Settings.getSyncHostType());
    dlg.setPostgrestUrl(global_Settings.getSyncPostgrestUrl());
    dlg.setEmail(global_Settings.getSyncEmail());
    dlg.setPassword(global_Settings.getSyncPassword());
    dlg.setEncryptionPhrase(global_Settings.getSyncEncryptionPhrase());
    dlg.setSupabaseKey(global_Settings.getSyncSupabaseKey());

    if (dlg.exec() != QDialog::Accepted)
        return;

    // Database path is always fixed to the app data location (profile-aware)
    const QString dbfile = AppSettings::dataLocation() + "/ProjectNotes.db";

    // Persist sync settings immediately (before openDatabase so it can read them)
    global_Settings.setSyncEnabled(dlg.syncEnabled());
    global_Settings.setSyncHostType(dlg.syncHostType());
    global_Settings.setSyncPostgrestUrl(dlg.postgrestUrl());
    global_Settings.setSyncEmail(dlg.email());
    global_Settings.setSyncPassword(dlg.password());
    global_Settings.setSyncEncryptionPhrase(dlg.encryptionPhrase());
    global_Settings.setSyncSupabaseKey(dlg.supabaseKey());

    // Create new database if the file does not yet exist
    if (!QFile::exists(dbfile))
        global_DBObjects.createDatabase(dbfile);

    openDatabase(dbfile);
}



void MainWindow::openDatabase(const QString& dbfile)
{
    if (!global_DBObjects.openDatabase(dbfile, mainConnectionName()))
        return;

    // Start sync if enabled
    if (global_Settings.getSyncEnabled()) {
        if (!m_syncApi)
            m_syncApi = new SqliteSyncPro(this);

        m_syncApi->setDatabasePath(dbfile);
        m_syncApi->setSyncHostType(global_Settings.getSyncHostType());
        m_syncApi->setPostgrestUrl(global_Settings.getSyncPostgrestUrl());
        m_syncApi->setEmail(global_Settings.getSyncEmail());
        m_syncApi->setPassword(global_Settings.getSyncPassword());
        m_syncApi->setEncryptionPhrase(global_Settings.getSyncEncryptionPhrase());
        m_syncApi->setSupabaseKey(global_Settings.getSyncSupabaseKey());

        m_syncApi->setDatabaseLock(&db_rwlock);

        if (m_syncApi->initialize()) {
            connect(m_syncApi, &SqliteSyncPro::rowChanged,
                    this, &MainWindow::onSyncRowChanged,
                    Qt::QueuedConnection);
            connect(m_syncApi, &SqliteSyncPro::syncCompleted,
                    this, [this](const SyncResult &result){
                        m_syncNetworkError = result.hasNetworkError();
                        global_DBObjects.updateDisplayData();
                        if (result.totalDecryptionFailures() > 0) {
                            QMessageBox::warning(this, tr("Cloud Sync"),
                                tr("One or more records could not be decrypted during sync. "
                                   "Your encryption phrase may be incorrect.\n\n"
                                   "Please verify the phrase via File > Cloud Sync Settings."));
                        }
                        m_syncApi->checkSyncStatus(result);
                    },
                    Qt::QueuedConnection);
            connect(m_syncApi, &SqliteSyncPro::syncStatusUpdated,
                    this, &MainWindow::onSyncStatusUpdated,
                    Qt::QueuedConnection);
        } else {
#ifdef QT_DEBUG
            qWarning() << "SqliteSyncPro initialize failed:" << m_syncApi->lastError();
#endif
            QMessageBox::warning(this, tr("Cloud Sync"),
                tr("Connection settings are invalid — unable to connect to the sync host.\n\n"
                   "Your settings have been saved. You can update them via File > Cloud Sync Settings."));
        }
    }

    // load and refresh all of the models in order of their dependancy relationships
    global_DBObjects.unfilteredpeoplemodel()->refresh();
    global_DBObjects.unfilteredclientsmodel()->refresh();
    global_DBObjects.unfilteredprojectslistmodel()->refresh();

    global_DBObjects.setGlobalSearches(false);

    global_DBObjects.allitemsmodel()->refresh();

    global_DBObjects.clientsmodel()->loadUserFilter(global_DBObjects.clientsmodel()->objectName());
    global_DBObjects.clientsmodel()->activateUserFilter(global_DBObjects.clientsmodel()->objectName());

    global_DBObjects.peoplemodel()->loadUserFilter(global_DBObjects.peoplemodel()->objectName());
    global_DBObjects.peoplemodel()->activateUserFilter(global_DBObjects.peoplemodel()->objectName());

    global_DBObjects.projectslistmodel()->loadUserFilter(global_DBObjects.projectslistmodel()->objectName());
    global_DBObjects.projectslistmodel()->activateUserFilter(global_DBObjects.projectslistmodel()->objectName());

    // assign all of the newly open models
    ui->pageProjectsList->setupModels(ui);
    ui->pageClients->setupModels(ui);
    ui->pagePeople->setupModels(ui);
    ui->pageProjectDetails->setupModels(ui);
    ui->pageItemDetails->setupModels(ui);
    ui->pageProjectNote->setupModels(ui);
    ui->pageSearch->setupModels(ui);
    ui->pageMasterItemList->setupModels(ui);

    navigateClearHistory();
    navigateToPage(ui->pageProjectsList, QVariant());

    setButtonAndMenuStates();

    // update button/menu states whenever a table row is clicked
    const auto tableviews = findChildren<TableView*>();
    for (TableView* tv : tableviews)
        connect(tv, &TableView::rowSelectionChanged, this, &MainWindow::setButtonAndMenuStates);

    // connect the search request event
    connect(global_DBObjects.peoplemodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.clientsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.projectslistmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.statusreportitemsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.projectteammembersmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.projectlocationsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.projectnotesmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.meetingattendeesmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.notesactionitemsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.trackeritemsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    connect(global_DBObjects.trackeritemscommentsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));

    // keep the Show Closed Projects menu checkbox in sync when the setting is changed programmatically
    connect(&global_DBObjects, &DatabaseObjects::showClosedProjectsChanged,
            this, [this](bool showClosed) {
                ui->actionClosed_Projects->setChecked(showClosed);
            });
}

void MainWindow::navigateToPage(BasePage* widget, QVariant recordId)
{
    if (navigateCurrentPage())
    {
        QString a = recordId.toString();
        QString b = navigateCurrentPage()->getRecordId().toString();

        if ( a.compare(b) == 0 && navigateCurrentPage() == widget)
            return;
    }

    if (navigateCurrentPage())
    {
        navigateCurrentPage()->saveState();
        navigateCurrentPage()->submitRecord();
    }

    // Clear quick search when leaving the current page
    if (m_quickSearchEdit && !m_quickSearchEdit->text().isEmpty())
    {
        if (navigateCurrentPage() && navigateCurrentPage()->getCurrentModel())
            navigateCurrentPage()->getCurrentModel()->setQuickSearch(QString());
        m_quickSearchEdit->blockSignals(true);
        m_quickSearchEdit->clear();
        m_quickSearchEdit->blockSignals(false);
    }

    widget->openRecord(recordId);

    ui->stackedWidget->setCurrentWidget(widget);

    widget->setPageTitle();

    HistoryNode* buttonnode = new HistoryNode(widget, recordId, widget->getHistoryText());

    // if in the middle of the button history chop off the remaining history
    while (m_navigationLocation < m_forwardBackHistory.count() - 1)
    {
        HistoryNode* hn = m_forwardBackHistory.pop();
        delete hn;
    }

    m_navigationLocation = m_forwardBackHistory.count();
    m_forwardBackHistory.push(buttonnode);

    buildHistory(buttonnode);

    buildPluginMenu(widget);

    setButtonAndMenuStates();

    navigateCurrentPage()->loadState();
}

void MainWindow::navigateBackward()
{
    if (m_navigationLocation > 0)
    {
        if (navigateCurrentPage())
        {
            navigateCurrentPage()->saveState();
            navigateCurrentPage()->submitRecord();
        }

        if (m_quickSearchEdit && !m_quickSearchEdit->text().isEmpty())
        {
            if (navigateCurrentPage() && navigateCurrentPage()->getCurrentModel())
                navigateCurrentPage()->getCurrentModel()->setQuickSearch(QString());
            m_quickSearchEdit->blockSignals(true);
            m_quickSearchEdit->clear();
            m_quickSearchEdit->blockSignals(false);
        }

        m_navigationLocation--;

        HistoryNode* hn = m_forwardBackHistory.at(m_navigationLocation);
        buildHistory(hn);

        BasePage* current = hn->m_page;
        QVariant record_id = hn->m_recordId;

        current->openRecord(record_id);

        ui->stackedWidget->setCurrentWidget(current);
        current->setPageTitle();

        buildPluginMenu(current);
    }

    setButtonAndMenuStates();
}

void MainWindow::navigateForward()
{
    if (m_navigationLocation < (m_forwardBackHistory.count() - 1) )
    {
        if (navigateCurrentPage())
        {
            navigateCurrentPage()->saveState();
            navigateCurrentPage()->submitRecord();
        }

        if (m_quickSearchEdit && !m_quickSearchEdit->text().isEmpty())
        {
            if (navigateCurrentPage() && navigateCurrentPage()->getCurrentModel())
                navigateCurrentPage()->getCurrentModel()->setQuickSearch(QString());
            m_quickSearchEdit->blockSignals(true);
            m_quickSearchEdit->clear();
            m_quickSearchEdit->blockSignals(false);
        }

        m_navigationLocation++;

        HistoryNode* hn = m_forwardBackHistory.at(m_navigationLocation);
        buildHistory(hn);

        BasePage* current = hn->m_page;
        QVariant record_id = hn->m_recordId;

        current->openRecord(record_id);

        ui->stackedWidget->setCurrentWidget(current);
        current->setPageTitle();

        buildPluginMenu(current);
    }

    setButtonAndMenuStates();
}

void MainWindow::buildHistory(HistoryNode* node)
{
    HistoryNode* hn = new HistoryNode(node->m_page, node->m_recordId, node->m_pagetitle);

    // remove this page if is in the past
    int nodecount = 0;
    while (nodecount < m_pageHistory.count())
    {
        if (m_pageHistory.at(nodecount)->equals(hn))
        {
            delete m_pageHistory.at(nodecount);
            m_pageHistory.remove(nodecount);
        }
        else
            nodecount++;
    }

    // if past the max delete the node
    if (m_pageHistory.count() > MAXHISTORYNODES)
    {
        HistoryNode* ohn = m_pageHistory.pop();
        delete ohn;
    }

    m_pageHistory.push(hn);
}

void MainWindow::cleanNavigationHistory(const QVariant& recordId)
{
    // Remove from page history menu stack
    int i = 0;
    while (i < m_pageHistory.count())
    {
        if (m_pageHistory.at(i)->m_recordId == recordId)
        {
            delete m_pageHistory.at(i);
            m_pageHistory.remove(i);
        }
        else
            i++;
    }

    // Remove from back/forward stack, adjusting m_navigationLocation for
    // any removed entries that were before the current position
    i = 0;
    while (i < m_forwardBackHistory.count())
    {
        if (m_forwardBackHistory.at(i)->m_recordId == recordId)
        {
            delete m_forwardBackHistory.at(i);
            m_forwardBackHistory.remove(i);
            if (i <= m_navigationLocation)
                m_navigationLocation--;
        }
        else
            i++;
    }

    // Clamp navigationLocation to valid range
    if (m_navigationLocation >= m_forwardBackHistory.count())
        m_navigationLocation = m_forwardBackHistory.count() - 1;

    // Rebuild the history menu and update button states
    ui->menuHistory->clear();
    int hi = m_pageHistory.count();
    while (hi)
    {
        hi--;
        HistoryNode* hn = m_pageHistory.at(hi);
        QString title = QString("%1 - %2").arg(hn->m_pagetitle, hn->m_timestamp.toString("MM/dd/yy hh:mm"));
        QAction* ha = ui->menuHistory->addAction(title, [hn, this](){ navigateToPage(hn->m_page, hn->m_recordId); });
        ha->setPriority(QAction::LowPriority);
    }

    setButtonAndMenuStates();
}

void MainWindow::CloseDatabase()
{
    // Shut down background sync before closing the database.
    // shutdown() is async; wait for syncStopped so the worker thread has released
    // the database connection before we call closeDatabase().
    if (m_syncApi) {
        // Block row-changed propagation; we don't care about incoming sync updates anymore
        disconnect(m_syncApi, &SqliteSyncPro::rowChanged, this, &MainWindow::onSyncRowChanged);

        QEventLoop loop;
        connect(m_syncApi, &SqliteSyncPro::syncStopped, &loop, &QEventLoop::quit);
        m_syncApi->shutdown();
        loop.exec();   // returns once the worker thread has exited cleanly

        m_syncApi->deleteLater();
        m_syncApi = nullptr;
    }

    // disconnect table row selection state updates
    const auto tableviews = findChildren<TableView*>();
    for (TableView* tv : tableviews)
        disconnect(tv, &TableView::rowSelectionChanged, this, &MainWindow::setButtonAndMenuStates);

    // disconnect the search request event
    disconnect(global_DBObjects.peoplemodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.clientsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.projectslistmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.statusreportitemsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.projectteammembersmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.projectlocationsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.projectnotesmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.meetingattendeesmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.notesactionitemsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.trackeritemsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));
    disconnect(global_DBObjects.trackeritemscommentsmodel(), SIGNAL(callKeySearch()), this, SLOT(on_actionSearch_triggered()));

    global_DBObjects.closeDatabase();
}


void MainWindow::on_actionClosed_Projects_triggered()
{
    global_DBObjects.setShowClosedProjects(ui->actionClosed_Projects->isChecked());
    global_DBObjects.setGlobalSearches(true);
}

void MainWindow::on_actionStatus_Bar_triggered()
{
    statusBar()->setVisible(!statusBar()->isVisible());
}

void MainWindow::on_actionFilter_triggered()
{
    TableView* curview = navigateCurrentPage()->getCurrentView();

    curview->filterDialog();
}

void MainWindow::on_actionClients_triggered()
{
    navigateToPage(ui->pageClients, QVariant());
}

void MainWindow::on_actionPeople_triggered()
{
    navigateToPage(ui->pagePeople, QVariant());
}

void MainWindow::on_actionProjects_triggered()
{
    navigateToPage(ui->pageProjectsList, QVariant());
}

void MainWindow::on_actionBack_triggered()
{
    navigateBackward();
}

void MainWindow::on_actionForward_triggered()
{
    navigateForward();
}

void MainWindow::on_actionNew_Item_triggered()
{
    if ( navigateCurrentPage() )
        navigateCurrentPage()->newRecord();
}


void MainWindow::on_actionOpen_Item_triggered()
{
    if ( navigateCurrentPage() )
    {
        navigateCurrentPage()->getCurrentView()->slotOpenRecord();
    }
}

void MainWindow::on_actionCopy_Item_triggered()
{
    if ( navigateCurrentPage() )
        navigateCurrentPage()->copyItem();
}

void MainWindow::on_actionDelete_Item_triggered()
{
    if ( navigateCurrentPage() )
    {
        QVariantList deletedIds = navigateCurrentPage()->getSelectedRecordIds();
        navigateCurrentPage()->deleteItem();
        for (const QVariant& id : deletedIds)
            cleanNavigationHistory(id);
    }
}

void MainWindow::slotOpen_ProjectDetails_triggered(QVariant recordId)
{
    navigateToPage(ui->pageProjectDetails, recordId);
}

void MainWindow::slotOpen_ItemDetails_triggered(QVariant recordId)
{
    navigateToPage(ui->pageItemDetails, recordId);
}

void MainWindow::slotOpen_ProjectNote_triggered(QVariant recordId)
{
    navigateToPage(ui->pageProjectNote, recordId);
}

void MainWindow::slotOpenTeamMember_triggered(QVariant recordId)
{
    navigateToPage(ui->pagePeople, recordId);
}

void MainWindow::slotOpenLocation_triggered(QVariant recordId)
{
    QModelIndex qmi = global_DBObjects.projectlocationsmodel()->findIndex(recordId, 0);
    QModelIndex qi = global_DBObjects.projectlocationsmodelproxy()->index(global_DBObjects.projectlocationsmodelproxy()->mapFromSource(qmi).row(), 2);  // usa a visible column

    QVariant location = ui->tableViewLocations->model()->data(ui->tableViewLocations->model()->index(qi.row(), 4));
    QVariant location_type = ui->tableViewLocations->model()->data(ui->tableViewLocations->model()->index(qi.row(), 2));

    if ( location_type == "Web Link" )
    {
        QDesktopServices::openUrl(QUrl(location.toString(), QUrl::TolerantMode));
    }
    else
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(location.toString()));
    }
}


void MainWindow::slotOpen_SearchResults_triggered(QVariant recordId)
{
    // find the selected search result
    QModelIndexList qil = ui->tableViewSearchResults->selectionModel()->selectedIndexes();
    QModelIndex qi = qil.first();

    QVariant data_type = ui->tableViewSearchResults->model()->data(ui->tableViewSearchResults->model()->index(qi.row(), 1));
    QVariant record_id = ui->tableViewSearchResults->model()->data(ui->tableViewSearchResults->model()->index(qi.row(), 0));
    QVariant fk_id = ui->tableViewSearchResults->model()->data(ui->tableViewSearchResults->model()->index(qi.row(), 13));

    if (data_type == tr("Client"))
    {
        navigateToPage(ui->pageClients, recordId);

        QModelIndex qmi = global_DBObjects.clientsmodel()->findIndex(record_id, 0);
        QModelIndex qi = global_DBObjects.clientsmodelproxy()->index(global_DBObjects.clientsmodelproxy()->mapFromSource(qmi).row(), 1);  // usa a visible column

        ui->tableViewClients->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewClients->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    else if (data_type == tr("People"))
    {
        navigateToPage(ui->pagePeople, recordId);

        QModelIndex qmi = global_DBObjects.peoplemodel()->findIndex(record_id, 0);
        QModelIndex qi = global_DBObjects.peoplemodelproxy()->index(global_DBObjects.peoplemodelproxy()->mapFromSource(qmi).row(), 1);  // usa a visible column

        ui->tableViewPeople->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewPeople->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    else if (data_type == tr("Project"))
    {
        navigateToPage(ui->pageProjectDetails, recordId);

        ui->tabWidgetProject->setCurrentIndex(0);
    }
    else if (data_type == tr("Project Notes"))
    {
        navigateToPage(ui->pageProjectDetails, fk_id);
        navigateToPage(ui->pageProjectNote, recordId);

        ui->tabWidgetNotes->setCurrentIndex(0);
    }
    else if (data_type == tr("Meeting Attendees"))
    {
        QVariant project_id = global_DBObjects.execute(QString("select project_id from project_notes where id = '%1'").arg(fk_id.toString()));

        navigateToPage(ui->pageProjectDetails, project_id);
        ui->tabWidgetProject->setCurrentIndex(4);

        QModelIndex qmi = global_DBObjects.projectnotesmodel()->findIndex(fk_id, 0);
        QModelIndex qi = global_DBObjects.projectnotesmodelproxy()->index(global_DBObjects.projectnotesmodelproxy()->mapFromSource(qmi).row(), 2);  // usa a visible column

        ui->tableViewProjectNotes->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewProjectNotes->scrollTo(qi, QAbstractItemView::PositionAtCenter);

        navigateToPage(ui->pageProjectNote, fk_id);
        ui->tabWidgetNotes->setCurrentIndex(1);

        qmi = global_DBObjects.meetingattendeesmodel()->findIndex(record_id, 0);
        qi = global_DBObjects.meetingattendeesmodelproxy()->index(global_DBObjects.meetingattendeesmodelproxy()->mapFromSource(qmi).row(), 2);  // usa a visible column

        ui->tableViewAtendees->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewAtendees->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    else if (data_type == tr("Project Locations"))
    {
        navigateToPage(ui->pageProjectDetails, fk_id);
        ui->tabWidgetProject->setCurrentIndex(3);

        QModelIndex qmi = global_DBObjects.projectlocationsmodel()->findIndex(record_id, 0);
        QModelIndex qi = global_DBObjects.projectlocationsmodelproxy()->index(global_DBObjects.projectlocationsmodelproxy()->mapFromSource(qmi).row(), 2);  // usa a visible column

        ui->tableViewLocations->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewLocations->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    else if (data_type == tr("Project Team"))
    {
        navigateToPage(ui->pageProjectDetails, fk_id);
        ui->tabWidgetProject->setCurrentIndex(1);

        QModelIndex qmi = global_DBObjects.projectteammembersmodel()->findIndex(record_id, 0);
        QModelIndex qi = global_DBObjects.projectteammembersmodelproxy()->index(global_DBObjects.projectteammembersmodelproxy()->mapFromSource(qmi).row(), 3);  // usa a visible column

        ui->tableViewTeam->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewTeam->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    else if (data_type == tr("Status Report Item"))
    {
        navigateToPage(ui->pageProjectDetails, fk_id);
        ui->tabWidgetProject->setCurrentIndex(0);

        QModelIndex qmi = global_DBObjects.statusreportitemsmodel()->findIndex(record_id, 0);
        QModelIndex qi = global_DBObjects.statusreportitemsmodelproxy()->index(global_DBObjects.statusreportitemsmodelproxy()->mapFromSource(qmi).row(), 1);  // usa a visible column

        ui->tableViewStatusReportItems->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewStatusReportItems->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
    else if (data_type == tr("Item Tracker") )
    {
        navigateToPage(ui->pageProjectDetails, fk_id);
        ui->tabWidgetProject->setCurrentIndex(2);

        QModelIndex qmi = global_DBObjects.trackeritemsmodel()->findIndex(recordId, 0);
        QModelIndex qi = global_DBObjects.trackeritemsmodelproxy()->index(global_DBObjects.trackeritemsmodelproxy()->mapFromSource(qmi).row(), 3);  // usa a visible column

        ui->tableViewTrackerItems->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewTrackerItems->scrollTo(qi, QAbstractItemView::PositionAtCenter);

        navigateToPage(ui->pageItemDetails, recordId);

    }
    else if (data_type == tr("Tracker Update") )
    {
        QVariant datakey = ui->tableViewSearchResults->model()->data(ui->tableViewSearchResults->model()->index(qi.row(), 14));

        navigateToPage(ui->pageProjectDetails, fk_id);
        ui->tabWidgetProject->setCurrentIndex(2);

        QModelIndex qmi = global_DBObjects.trackeritemsmodel()->findIndex(datakey, 0);
        QModelIndex qi = global_DBObjects.trackeritemsmodelproxy()->index(global_DBObjects.trackeritemsmodelproxy()->mapFromSource(qmi).row(), 3);  // usa a visible column

        ui->tableViewTrackerItems->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewTrackerItems->scrollTo(qi, QAbstractItemView::PositionAtCenter);

        navigateToPage(ui->pageItemDetails, datakey);

        qmi = global_DBObjects.trackeritemscommentsmodel()->findIndex(record_id, 0);
        qi = global_DBObjects.trackeritemscommentsmodelproxy()->index(global_DBObjects.trackeritemscommentsmodelproxy()->mapFromSource(qmi).row(), 3);  // usa a visible column

        ui->tableViewComments->selectionModel()->select(qi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->tableViewComments->scrollTo(qi, QAbstractItemView::PositionAtCenter);
    }
}

void MainWindow::on_actionInternal_Items_triggered()
{
    global_DBObjects.setShowInternalItems(ui->actionInternal_Items->isChecked());
    global_DBObjects.setGlobalSearches(true);

    setButtonAndMenuStates();
}

void MainWindow::on_actionResolved_Tracker_Action_Items_triggered()
{
    global_DBObjects.setShowResolvedTrackerItems(!ui->actionResolved_Tracker_Action_Items->isChecked());

    // filter tracker items by status: checked = show only New and Assigned; unchecked = show all
    if (ui->actionResolved_Tracker_Action_Items->isChecked())
    {
        global_DBObjects.trackeritemsmodel()->setFilter(9, "New,Assigned", SqlQueryModel::In);
        global_DBObjects.allitemsmodel()->setFilter(9, "New,Assigned", SqlQueryModel::In);
    }
    else
    {
        global_DBObjects.trackeritemsmodel()->clearFilter(9);
        global_DBObjects.allitemsmodel()->clearFilter(9);
    }

    global_DBObjects.trackeritemsmodel()->refresh();
    global_DBObjects.allitemsmodel()->refresh();

    setButtonAndMenuStates();
}

void MainWindow::on_actionPreferences_triggered()
{
    m_preferencesDialog->show();
}

void MainWindow::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(m_actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(m_actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(m_actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::indent()
{
    modifyIndentation(1);
}

void MainWindow::unindent()
{
    modifyIndentation(-1);
}

void MainWindow::textAlign(QAction *a)
{
    if (a == m_actionAlignLeft)
        ui->textEditNotes->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (a == m_actionAlignCenter)
        ui->textEditNotes->setAlignment(Qt::AlignHCenter);
    else if (a == m_actionAlignRight)
        ui->textEditNotes->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else if (a == m_actionAlignJustify)
        ui->textEditNotes->setAlignment(Qt::AlignJustify);
}

void MainWindow::textColor()
{
    QColor col = QColorDialog::getColor(ui->textEditNotes->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}


void MainWindow::setupTextActions()
{
    QToolBar *tb = ui->toolBarFormat;
    QToolBar *tbs = ui->toolBarSearch;

    QMenu *menu = ui->menuFormat;

    const QIcon boldIcon = QIcon(rsrcPath + "/textbold.png");
    m_actionTextBold = menu->addAction(boldIcon, tr("&Bold"), this, &MainWindow::textBold);
    m_actionTextBold->setStatusTip("Bold text");
    m_actionTextBold->setShortcut(Qt::CTRL | Qt::Key_B);
    m_actionTextBold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    m_actionTextBold->setFont(bold);
    tb->addAction(m_actionTextBold);
    m_actionTextBold->setCheckable(true);

    const QIcon italicIcon = QIcon(rsrcPath + "/textitalic.png");
    m_actionTextItalic = menu->addAction(italicIcon, tr("&Italic"), this, &MainWindow::textItalic);
    m_actionTextItalic->setStatusTip("Italic text");
    m_actionTextItalic->setPriority(QAction::LowPriority);
    m_actionTextItalic->setShortcut(Qt::CTRL | Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    m_actionTextItalic->setFont(italic);
    tb->addAction(m_actionTextItalic);
    m_actionTextItalic->setCheckable(true);

    const QIcon underlineIcon = QIcon(rsrcPath + "/textunder.png");
    m_actionTextUnderline = menu->addAction(underlineIcon, tr("&Underline"), this, &MainWindow::textUnderline);
    m_actionTextUnderline->setStatusTip("Underline text");
    m_actionTextUnderline->setShortcut(Qt::CTRL | Qt::Key_U);
    m_actionTextUnderline->setPriority(QAction::LowPriority);
    QFont underline;
    underline.setUnderline(true);
    m_actionTextUnderline->setFont(underline);
    tb->addAction(m_actionTextUnderline);
    m_actionTextUnderline->setCheckable(true);

    menu->addSeparator();

    const QIcon leftIcon = QIcon(rsrcPath + "/textleft.png");
    m_actionAlignLeft = new QAction(leftIcon, tr("&Left"), this);
    m_actionAlignLeft->setStatusTip("Left align text");
    m_actionAlignLeft->setShortcut(Qt::CTRL | Qt::Key_L);
    m_actionAlignLeft->setCheckable(true);
    m_actionAlignLeft->setPriority(QAction::LowPriority);
    const QIcon centerIcon = QIcon(rsrcPath + "/textcenter.png");
    m_actionAlignCenter = new QAction(centerIcon, tr("C&enter"), this);
    m_actionAlignCenter->setStatusTip("Center align text");
    m_actionAlignCenter->setShortcut(Qt::CTRL | Qt::Key_E);
    m_actionAlignCenter->setCheckable(true);
    m_actionAlignCenter->setPriority(QAction::LowPriority);
    const QIcon rightIcon = QIcon(rsrcPath + "/textright.png");
    m_actionAlignRight = new QAction(rightIcon, tr("&Right"), this);
    m_actionAlignRight->setStatusTip("Right align text");
    m_actionAlignRight->setShortcut(Qt::CTRL | Qt::Key_R);
    m_actionAlignRight->setCheckable(true);
    m_actionAlignRight->setPriority(QAction::LowPriority);
    const QIcon fillIcon = QIcon(rsrcPath + "/textjustify.png");
    m_actionAlignJustify = new QAction(fillIcon, tr("&Justify"), this);
    m_actionAlignJustify->setStatusTip("Justify text");
    m_actionAlignJustify->setShortcut(Qt::CTRL | Qt::Key_J);
    m_actionAlignJustify->setCheckable(true);
    m_actionAlignJustify->setPriority(QAction::LowPriority);
    const QIcon indentMoreIcon = QIcon(rsrcPath + "/format-indent-more.png");
    m_actionIndentMore = menu->addAction(indentMoreIcon, tr("&Indent"), this, &MainWindow::indent);
    m_actionIndentMore->setStatusTip("Indent text");
    m_actionIndentMore->setShortcut(Qt::CTRL | Qt::Key_BracketRight);
    m_actionIndentMore->setPriority(QAction::LowPriority);
    const QIcon indentLessIcon = QIcon(rsrcPath + "/format-indent-less.png");
    m_actionIndentLess = menu->addAction(indentLessIcon, tr("&Unindent"), this, &MainWindow::unindent);
    m_actionIndentLess->setStatusTip("Unindent text");
    m_actionIndentLess->setShortcut(Qt::CTRL | Qt::Key_BracketLeft);
    m_actionIndentLess->setPriority(QAction::LowPriority);

    // Make sure the alignLeft  is always left of the alignRight
    QActionGroup *alignGroup = new QActionGroup(this);
    connect(alignGroup, &QActionGroup::triggered, this, &MainWindow::textAlign);

    if (QApplication::isLeftToRight())
    {
        alignGroup->addAction(m_actionAlignLeft);
        alignGroup->addAction(m_actionAlignCenter);
        alignGroup->addAction(m_actionAlignRight);
    }
    else
    {
        alignGroup->addAction(m_actionAlignRight);
        alignGroup->addAction(m_actionAlignCenter);
        alignGroup->addAction(m_actionAlignLeft);
    }
    alignGroup->addAction(m_actionAlignJustify);

    tb->addActions(alignGroup->actions());
    menu->addActions(alignGroup->actions());
    tb->addAction(m_actionIndentMore);
    tb->addAction(m_actionIndentLess);
    menu->addAction(m_actionIndentMore);
    menu->addAction(m_actionIndentLess);

    menu->addSeparator();

    QPixmap pix(16, 16);
    pix.fill(Qt::black);
    m_actionTextColor = menu->addAction(pix, tr("&Color..."), this, &MainWindow::textColor);
    m_actionTextColor->setStatusTip("Color text");
    tb->addAction(m_actionTextColor);

    menu->addSeparator();

    tb->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    m_comboBoxStyle = new ComboBox(tb);
    tb->addWidget(m_comboBoxStyle);
    m_comboBoxStyle->addItem("Standard");
    m_comboBoxStyle->addItem("Bullet List (Disc)");
    m_comboBoxStyle->addItem("Bullet List (Circle)");
    m_comboBoxStyle->addItem("Bullet List (Square)");
    m_comboBoxStyle->addItem("Task List (Unchecked)");
    m_comboBoxStyle->addItem("Task List (Checked)");
    m_comboBoxStyle->addItem("Ordered List (Decimal)");
    m_comboBoxStyle->addItem("Ordered List (Alpha lower)");
    m_comboBoxStyle->addItem("Ordered List (Alpha upper)");
    m_comboBoxStyle->addItem("Ordered List (Roman lower)");
    m_comboBoxStyle->addItem("Ordered List (Roman upper)");
    m_comboBoxStyle->addItem("Heading 1");
    m_comboBoxStyle->addItem("Heading 2");
    m_comboBoxStyle->addItem("Heading 3");
    m_comboBoxStyle->addItem("Heading 4");
    m_comboBoxStyle->addItem("Heading 5");
    m_comboBoxStyle->addItem("Heading 6");

    connect(m_comboBoxStyle, QOverload<int>::of(&ComboBox::activated), this, &MainWindow::textStyle);

    m_comboBoxFont = new QFontComboBox(tb);
    m_comboBoxFont->setFontFilters(QFontComboBox::AllFonts);
    m_comboBoxFont->setWritingSystem(QFontDatabase::Any);
    m_comboBoxFont->setCurrentFont(QFont("Arial", 11));
    tb->addWidget(m_comboBoxFont);
    connect(m_comboBoxFont, &ComboBox::textActivated, this, &MainWindow::textFamily);

    m_comboBoxSize = new ComboBox(tb);
    m_comboBoxSize->setObjectName("comboSize");
    tb->addWidget(m_comboBoxSize);
    m_comboBoxSize->setEditable(true);

    const QList<int> standardSizes = QFontDatabase::standardSizes();
    for (int size : standardSizes)
        m_comboBoxSize->addItem(QString::number(size));
    // m_comboBoxSize->setCurrentIndex(standardSizes.indexOf(QApplication::font().pointSize()));
    m_comboBoxSize->setCurrentIndex(m_comboBoxSize->findText("11"));

    connect(m_comboBoxSize, &ComboBox::textActivated, this, &MainWindow::textSize);

    // Quick search — pushed to the right end of the toolbar
    // Create the search box
    m_quickSearchEdit = new QLineEdit(tb);
    m_quickSearchEdit->setPlaceholderText(tr("Search…"));
    m_quickSearchEdit->setMaximumWidth(300);
    m_quickSearchEdit->setMinimumWidth(200);        // prevents it from getting too small
    m_quickSearchEdit->setClearButtonEnabled(true);

    // Container for the search box + right margin
    QWidget* searchContainer = new QWidget(tb);
    QHBoxLayout* searchLay = new QHBoxLayout(searchContainer);

    // Make sure the spacer is strong
    searchContainer->setMinimumWidth(0);

    searchLay->addStretch(1);
    searchLay->addWidget(m_quickSearchEdit);
    searchLay->setContentsMargins(0, 0, 5, 0);     // ← This creates space on the right
    searchLay->setSpacing(0);

    // Big expanding spacer (pushes everything to the right)
    QWidget* spacer = new QWidget(tb);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Add to toolbar in correct order
    tbs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    tbs->addWidget(spacer);           // pushes search to the right
    tbs->addWidget(searchContainer);  // search box with right padding

    connect(m_quickSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onQuickSearchChanged);
}

void MainWindow::onQuickSearchChanged(const QString& text)
{
    BasePage* page = navigateCurrentPage();
    if (!page)
        return;
    SortFilterProxyModel* model = page->getCurrentModel();
    if (!model)
        return;
    model->setQuickSearch(text);
}

void MainWindow::textStyle(int styleIndex)
{
    QTextCursor cursor = ui->textEditNotes->textCursor();

    QTextListFormat::Style style = QTextListFormat::ListStyleUndefined;
    QTextBlockFormat::MarkerType marker = QTextBlockFormat::MarkerType::NoMarker;

    switch (styleIndex)
    {
    case 1:
        style = QTextListFormat::ListDisc;
        break;
    case 2:
        style = QTextListFormat::ListCircle;
        break;
    case 3:
        style = QTextListFormat::ListSquare;
        break;
    case 4:
        if (cursor.currentList())
            style = cursor.currentList()->format().style();
        else
            style = QTextListFormat::ListDisc;

        marker = QTextBlockFormat::MarkerType::Unchecked;
        break;
    case 5:
        if (cursor.currentList())
            style = cursor.currentList()->format().style();
        else
            style = QTextListFormat::ListDisc;

        marker = QTextBlockFormat::MarkerType::Checked;
        break;
    case 6:
        style = QTextListFormat::ListDecimal;
        break;
    case 7:
        style = QTextListFormat::ListLowerAlpha;
        break;
    case 8:
        style = QTextListFormat::ListUpperAlpha;
        break;
    case 9:
        style = QTextListFormat::ListLowerRoman;
        break;
    case 10:
        style = QTextListFormat::ListUpperRoman;
        break;
    default:
        break;
    }

    cursor.beginEditBlock();

    QTextBlockFormat blockFmt = cursor.blockFormat();

    if (style == QTextListFormat::ListStyleUndefined)
    {
        blockFmt.setObjectIndex(-1);
        int headingLevel = styleIndex >= 11 ? styleIndex - 11 + 1 : 0; // H1 to H6, or Standard
        blockFmt.setHeadingLevel(headingLevel);
        cursor.setBlockFormat(blockFmt);

        int sizeAdjustment = headingLevel ? 4 - headingLevel : 0; // H1 to H6: +3 to -2
        QTextCharFormat fmt;
        fmt.setFontWeight(headingLevel ? QFont::Bold : QFont::Normal);
        fmt.setProperty(QTextFormat::FontSizeAdjustment, sizeAdjustment);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.mergeCharFormat(fmt);
        ui->textEditNotes->mergeCurrentCharFormat(fmt);
    }
    else
    {
        blockFmt.setMarker(marker);
        cursor.setBlockFormat(blockFmt);
        QTextListFormat listFmt;
        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        }
        else
        {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }
        listFmt.setStyle(style);
        cursor.createList(listFmt);
    }

    cursor.endEditBlock();
}

void MainWindow::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    // don't use this method is is broken fmt.setFontFamily(f);
    fmt.setFont(f);
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui->textEditNotes->textCursor();

    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    cursor.mergeCharFormat(format);
}

void MainWindow::modifyIndentation(int amount)
{
    QTextCursor cursor = ui->textEditNotes->textCursor();
    cursor.beginEditBlock();
    if (cursor.currentList())
    {
        QTextListFormat listFmt = cursor.currentList()->format();
        // See whether the line above is the list we want to move this item into,
        // or whether we need a new list.
        QTextCursor above(cursor);
        above.movePosition(QTextCursor::Up);
        if (above.currentList() && listFmt.indent() + amount == above.currentList()->format().indent())
        {
            above.currentList()->add(cursor.block());
        }
        else
        {
            listFmt.setIndent(listFmt.indent() + amount);
            cursor.createList(listFmt);
        }
    }
    else
    {
        QTextBlockFormat blockFmt = cursor.blockFormat();
        blockFmt.setIndent(blockFmt.indent() + amount);
        cursor.setBlockFormat(blockFmt);
    }
    cursor.endEditBlock();
}

void MainWindow::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    m_actionTextColor->setIcon(pix);
}

void MainWindow::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0)
    {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void MainWindow::cursorPositionChanged()
{
    alignmentChanged(ui->textEditNotes->alignment());
    QTextList *list = ui->textEditNotes->textCursor().currentList();
    if (list) {
        switch (list->format().style()) {
        case QTextListFormat::ListDisc:
            m_comboBoxStyle->setCurrentIndex(1);
            break;
        case QTextListFormat::ListCircle:
            m_comboBoxStyle->setCurrentIndex(2);
            break;
        case QTextListFormat::ListSquare:
            m_comboBoxStyle->setCurrentIndex(3);
            break;
        case QTextListFormat::ListDecimal:
            m_comboBoxStyle->setCurrentIndex(6);
            break;
        case QTextListFormat::ListLowerAlpha:
            m_comboBoxStyle->setCurrentIndex(7);
            break;
        case QTextListFormat::ListUpperAlpha:
            m_comboBoxStyle->setCurrentIndex(8);
            break;
        case QTextListFormat::ListLowerRoman:
            m_comboBoxStyle->setCurrentIndex(9);
            break;
        case QTextListFormat::ListUpperRoman:
            m_comboBoxStyle->setCurrentIndex(10);
            break;
        default:
            m_comboBoxStyle->setCurrentIndex(-1);
            break;
        }
    }
    else
    {
        int headingLevel = ui->textEditNotes->textCursor().blockFormat().headingLevel();
        m_comboBoxStyle->setCurrentIndex(headingLevel ? headingLevel + 10 : 0);
    }
}

void MainWindow::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft)
        m_actionAlignLeft->setChecked(true);
    else if (a & Qt::AlignHCenter)
        m_actionAlignCenter->setChecked(true);
    else if (a & Qt::AlignRight)
        m_actionAlignRight->setChecked(true);
    else if (a & Qt::AlignJustify)
        m_actionAlignJustify->setChecked(true);
}

void MainWindow::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void MainWindow::fontChanged(const QFont &f)
{
    m_comboBoxFont->setCurrentIndex(m_comboBoxFont->findText(QFontInfo(f).family()));
    m_comboBoxSize->setCurrentIndex(m_comboBoxSize->findText(QString::number(f.pointSize())));
    m_actionTextBold->setChecked(f.bold());
    m_actionTextItalic->setChecked(f.italic());
    m_actionTextUnderline->setChecked(f.underline());
}


void MainWindow::on_actionUndo_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "TextEdit")
        (dynamic_cast<TextEdit*>(fw))->undo();
    else if (cn == "PlainTextEdit")
        (dynamic_cast<PlainTextEdit*>(fw))->undo();
    else if (cn == "QLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->undo();
    else if (cn == "QExpandingLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->undo();
    else if (cn == "DateEditEx")
        (dynamic_cast<DateEditEx*>(fw))->getLineEdit()->undo();
    else if (cn == "ComboBox")
        (dynamic_cast<ComboBox*>(fw))->lineEdit()->undo();
}


void MainWindow::on_actionRedo_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "TextEdit")
        (dynamic_cast<TextEdit*>(fw))->redo();
    else if (cn == "PlainTextEdit")
        (dynamic_cast<PlainTextEdit*>(fw))->redo();
    else if (cn == "QLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->redo();
    else if (cn == "QExpandingLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->redo();
    else if (cn == "DateEditEx")
        (dynamic_cast<DateEditEx*>(fw))->getLineEdit()->redo();
    else if (cn == "ComboBox")
        (dynamic_cast<ComboBox*>(fw))->lineEdit()->redo();
}


void MainWindow::on_actionCopy_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "PlainTextEdit")
        (dynamic_cast<PlainTextEdit*>(fw))->copy();
    else if (cn == "TextEdit")
        (dynamic_cast<TextEdit*>(fw))->copy();
    else if (cn == "QLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->copy();
    else if (cn == "QExpandingLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->copy();
    else if (cn == "DateEditEx")
        (dynamic_cast<DateEditEx*>(fw))->getLineEdit()->copy();
    else if (cn == "ComboBox")
        (dynamic_cast<ComboBox*>(fw))->lineEdit()->copy();
}


void MainWindow::on_actionCut_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "TextEdit")
        (dynamic_cast<TextEdit*>(fw))->cut();
    else if (cn == "PlainTextEdit")
        (dynamic_cast<PlainTextEdit*>(fw))->cut();
    else if (cn == "QLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->cut();
    else if (cn == "QExpandingLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->cut();
    else if (cn == "DateEditEx")
        (dynamic_cast<DateEditEx*>(fw))->getLineEdit()->cut();
    else if (cn == "ComboBox")
        (dynamic_cast<ComboBox*>(fw))->lineEdit()->cut();
}


void MainWindow::on_actionPaste_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "TextEdit")
        (dynamic_cast<TextEdit*>(fw))->paste();
    else if (cn == "PlainTextEdit")
        (dynamic_cast<QPlainTextEdit*>(fw))->paste();
    else if (cn == "QLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->paste();
    else if (cn == "QExpandingLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->paste();
    else if (cn == "DateEditEx")
        (dynamic_cast<DateEditEx*>(fw))->getLineEdit()->paste();
    else if (cn == "ComboBox")
        (dynamic_cast<ComboBox*>(fw))->lineEdit()->paste();
}

void MainWindow::on_actionDelete_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "TextEdit")
        (dynamic_cast<TextEdit*>(fw))->textCursor().insertText("");
    else if (cn == "PlainTextEdit")
        (dynamic_cast<PlainTextEdit*>(fw))->textCursor().insertText("");
    else if (cn == "QLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->backspace();
    else if (cn == "QExpandingLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->backspace();
    else if (cn == "DateEditEx")
        (dynamic_cast<DateEditEx*>(fw))->getLineEdit()->copy();
    else if (cn == "ComboBox")
        (dynamic_cast<ComboBox*>(fw))->lineEdit()->backspace();
}

void MainWindow::on_actionSelect_All_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "TextEdit")
        (dynamic_cast<TextEdit*>(fw))->selectAll();
    else if (cn == "PlainTextEdit")
        (dynamic_cast<PlainTextEdit*>(fw))->selectAll();
    else if (cn == "QLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->selectAll();
    else if (cn == "QExpandingLineEdit")
        (dynamic_cast<QLineEdit*>(fw))->selectAll();
    else if (cn == "DateEditEx")
        (dynamic_cast<DateEditEx*>(fw))->getLineEdit()->selectAll();
    else if (cn == "ComboBox")
        (dynamic_cast<ComboBox*>(fw))->lineEdit()->selectAll();
}

void MainWindow::on_actionSpell_Check_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "TextEdit" || cn == "PlainTextEdit")
    {
        SpellCheckDialog spellcheck_dialog(this);
        spellcheck_dialog.spellCheck(fw);
    }
}

void MainWindow::on_actionFind_triggered()
{
    QWidget* fw = this->focusWidget();
    const QLatin1StringView cn(fw ? fw->metaObject()->className() : "");

    if (cn == "TextEdit")
        m_findReplaceDialog->showReplaceWindow(dynamic_cast<QTextEdit*>(fw));
    else if (cn == "PlainTextEdit")
        m_findReplaceDialog->showReplaceWindow(dynamic_cast<QPlainTextEdit*>(fw));
    else if (cn == "QLineEdit")
        m_findReplaceDialog->showReplaceWindow(dynamic_cast<QLineEdit*>(fw));
    else if (cn == "ComboBox")
        m_findReplaceDialog->showReplaceWindow(dynamic_cast<ComboBox*>(fw)->lineEdit());
}

void MainWindow::on_actionSearch_triggered()
{
    navigateToPage(ui->pageSearch, QVariant());
}

void MainWindow::on_actionMasterItemList_triggered()
{
    navigateToPage(ui->pageMasterItemList, QVariant());
}

void MainWindow::on_pushButtonSearch_clicked()
{
    global_DBObjects.searchresultsmodel()->PerformSearch(ui->plainTextEditSearchText->toPlainText());
}

void MainWindow::on_actionView_LogView_triggered()
{
    if (ui->actionView_LogView->isChecked())
    {
        if (m_logviewDialog == nullptr)
        {
            m_logviewDialog = new LogViewer(this);
            connect(m_logviewDialog, &LogViewer::closed, this, [this]() {
                ui->actionView_LogView->setChecked(false);
            });
        }

        m_logviewDialog->show();
    }
    else
    {
        if (m_logviewDialog)
        {
            m_logviewDialog->hide();
            delete m_logviewDialog;
            m_logviewDialog = nullptr;
        }
    }
}

void MainWindow::on_actionXML_Import_triggered()
{
    // choose the file
    QString xmlfile = QFileDialog::getOpenFileName(this, tr("Import XML from file"), QString(), tr("XML File (*.xml)"));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    if (!xmlfile.isEmpty())
    {
        QFile infile(xmlfile);

        if (!infile.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox::critical(this, tr("Open Failed"), infile.errorString());
            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
            return;
        }

        QDomDocument xmldoc;
        xmldoc.setContent(&infile);

        if (!global_DBObjects.importXMLDoc(xmldoc))
        {
            QMessageBox::critical(this, tr("Open Failed"), "Parsing XML file failed.");
            infile.close();
            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
            return;
        }

        infile.close();

        global_DBObjects.updateDisplayData();
    }

    QApplication::restoreOverrideCursor();
    QApplication::processEvents();
}

void MainWindow::on_actionXML_Export_triggered()
{
    navigateCurrentPage()->submitRecord();  // make sure it is saved before calling an export

    TableView* curview = navigateCurrentPage()->getCurrentView();
    bool sel = curview->selectionModel()->hasSelection();

    if (curview && sel)
    {
        curview->slotExportRecord();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dlg;

    dlg.exec();
}


void MainWindow::on_actionHelp_triggered()
{
    QDesktopServices::openUrl(QUrl("https://projectnotes.readthedocs.io/", QUrl::TolerantMode));
}

void MainWindow::on_actionWhat_s_New_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/kestermckinney/ProjectNotes/wiki/Release%20Notes", QUrl::TolerantMode));
}

void MainWindow::on_actionIncrease_Font_Size_triggered()
{
    QFont af = QApplication::font();
    af.setPointSize(af.pointSize() + 1);
    QApplication::setFont(af);

    global_Settings.setStoredInt("DefaultFontSize",  QApplication::font().pointSize());

    QList<QWidget*> subwidgets = this->findChildren<QWidget*>();
    QListIterator<QWidget*> it(subwidgets); // iterate through the list of widgets
    QWidget *awiget;

    while (it.hasNext()) {
        awiget = it.next(); // take each widget in the list

        if ( QString(awiget->metaObject()->className()).contains("TableView") )
        {
            qobject_cast<QTableView*>(awiget)->resizeColumnsToContents();
            qobject_cast<QTableView*>(awiget)->resizeRowsToContents();
        }
    }

    global_Settings.setStoredInt("DefaultFontSize",  QApplication::font().pointSize());
}


void MainWindow::on_actionDecrease_Font_Size_triggered()
{
    QFont af = QApplication::font();
    af.setPointSize(af.pointSize() - 1);
    QApplication::setFont(af);

    QList<QWidget*> subwidgets = this->findChildren<QWidget*>();
    QListIterator<QWidget*> it(subwidgets); // iterate through the list of widgets
    QWidget *awiget;

    while (it.hasNext()) {
        awiget = it.next(); // take each widget in the list

        if ( QString(awiget->metaObject()->className()).contains("TableView") )
        {
            qobject_cast<QTableView*>(awiget)->resizeColumnsToContents();
            qobject_cast<QTableView*>(awiget)->resizeRowsToContents();
        }
    }

    global_Settings.setStoredInt("DefaultFontSize",  QApplication::font().pointSize());
}

void MainWindow::onPluginLoaded(const QString& pluginpath)
{
    HistoryNode* hn = (m_navigationLocation >= 0 ? m_forwardBackHistory.at(m_navigationLocation) : nullptr);
    BasePage* current = hn ? hn->m_page : nullptr;

    buildPluginMenu(dynamic_cast<BasePage*>(current));
}

void MainWindow::onPluginUnLoaded(const QString& pluginpath)
{
    HistoryNode* hn = (m_navigationLocation >= 0 ? m_forwardBackHistory.at(m_navigationLocation) : nullptr);
    BasePage* current = hn ? hn->m_page : nullptr;

    buildPluginMenu(dynamic_cast<BasePage*>(current));
}

void MainWindow::onRefreshRequested()
{
    global_DBObjects.updateDisplayData();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if (navigateCurrentPage())
    {
        navigateCurrentPage()->saveState();
        navigateCurrentPage()->submitRecord();
    }

    int loaded_count = m_pluginManager->loadedCount();

    if (loaded_count)
    {
        m_pluginManager->unloadAll();

        ui->centralwidget->setEnabled(false);
        ui->menubar->setEnabled(false);
        ui->toolBarEdit->setEnabled(false);
        ui->toolBarFormat->setEnabled(false);
        ui->toolBarNavigator->setEnabled(false);

        m_waitForThreadsTimer = new QTimer();
        connect(m_waitForThreadsTimer, &QTimer::timeout, this, &MainWindow::onTimerWaitForThreads);
        m_waitForThreadsTimer->start(800);

        event->ignore();
        return;
    }

    event->accept();
}

void MainWindow::showEvent(QShowEvent *event)
{
    global_Settings.getWindowState(objectName(), this);
    QMainWindow::showEvent(event);
}

void MainWindow::hideEvent(QHideEvent *event)
{
    global_Settings.setWindowState(objectName(), this);
    QMainWindow::hideEvent(event);
}

static int fan_index = 0;

void MainWindow::onTimerWaitForThreads()
{
    const char* fan = "-\\|/";
    fan_index = (fan_index + 1) % 4;

    int loaded_count = m_pluginManager->loadedCount();

    if (loaded_count == 0)
    {
        // Stop both timers before closing so neither fires again during teardown.
        m_waitForThreadsTimer->stop();
        m_pluginManager->stopWatcher();
        this->close();  // once all plugins are unloaded we can quit
    }

    QStringList activeNames;
    for (const Plugin* p : m_pluginManager->plugins())
        if (p->loaded())
            activeNames.append(p->pluginname());

    ui->statusbar->showMessage(QString("Waiting for plugins to finish... %1  %2 plugins are still active.").arg(fan[fan_index]).arg(loaded_count));
    ui->statusbar->setToolTip(activeNames.join(QStringLiteral("\n")));
}

void MainWindow::onSyncRowChanged(const QString& tableName, const QString& id)
{
    global_DBObjects.pushRowChange(tableName, id, KeyColumnChange::Update);
}

void MainWindow::onSyncStatusUpdated(int percentComplete, qint64 pendingPush, qint64 pendingPull)
{
    if (!m_syncProgressBar)
        return;

    if (!m_syncApi || !m_syncApi->isInitialized()) {
        m_syncProgressBar->hide();
        m_syncNetworkErrorLabel->hide();
        return;
    }

    if (m_syncNetworkError) {
        m_syncProgressBar->hide();
        m_syncNetworkErrorLabel->show();
        return;
    }

    m_syncNetworkErrorLabel->hide();
    m_syncProgressBar->setValue(percentComplete);

    if (percentComplete < 100) {
        m_syncProgressBar->setToolTip(
            tr("%1% Database Synced, pulling %2 and pushing %3 records")
                .arg(percentComplete)
                .arg(pendingPull)
                .arg(pendingPush));
        m_syncProgressBar->show();
    } else {
        m_syncProgressBar->hide();
    }
}

void MainWindow::onNetworkReachabilityChanged(QNetworkInformation::Reachability reachability)
{
    bool networkDown = (reachability == QNetworkInformation::Reachability::Disconnected);

    if (networkDown == m_syncNetworkError)
        return; // state hasn't changed — nothing to do

    m_syncNetworkError = networkDown;

    if (!m_syncProgressBar || !m_syncNetworkErrorLabel)
        return;

    if (networkDown) {
        // Show the icon immediately — don't wait for the next sync cycle to fail.
        if (m_syncApi && m_syncApi->isInitialized()) {
            m_syncProgressBar->hide();
            m_syncNetworkErrorLabel->show();
        }
    } else {
        // Network is back: hide the icon and kick the worker out of its backoff
        // sleep so the next sync cycle starts right away.
        m_syncNetworkErrorLabel->hide();
        if (m_syncApi && m_syncApi->isInitialized())
            m_syncApi->retryNow();
    }
}


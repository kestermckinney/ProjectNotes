// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "combobox.h"

#include <QMainWindow>
#include <QStringListModel>
#include <QStack>
#include <QTextCharFormat>
#include <QTimer>
#include <QComboBox>
#include <functional>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QProgressBar>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include "databaseobjects.h"
#include "appsettings.h"
#include "basepage.h"
#include "preferencesdialog.h"
#include "spellcheckdialog.h"
#include "findreplacedialog.h"
#include "logviewer.h"
#include "pluginmanager.h"
#include "sqlitesyncpro.h"

#define MAXHISTORYNODES 20

class HistoryNode
{
 public:
    void stamp() { m_timestamp = QDateTime::currentDateTime(); }
    bool equals(HistoryNode* node) {
        return node->m_page == m_page &&
               (node->m_pagetitle.compare(m_pagetitle) == 0) &&
               (node->m_recordId == m_recordId); }

    HistoryNode(BasePage* page, QVariant recordId, QString pagetitle) { m_page = page; m_recordId = recordId; m_pagetitle = pagetitle; stamp(); }
    BasePage* m_page;
    QVariant m_recordId;
    QString m_pagetitle;
    QDateTime m_timestamp;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void navigateToPage(BasePage* widget, QVariant recordId);
    void navigateForward();
    void navigateBackward();
    bool navigateAtEnd() { return (m_navigationLocation == (m_forwardBackHistory.count() - 1)); }
    bool navigateAtStart() { return (m_navigationLocation <= 0); }
    void buildHistory(HistoryNode* node);
    void navigateClearHistory() { m_navigationLocation = -1; qDeleteAll(m_forwardBackHistory); m_forwardBackHistory.clear(); }
    void cleanNavigationHistory(const QVariant& recordId);
    BasePage* navigateCurrentPage() { return ((m_forwardBackHistory.count() - 1) < m_navigationLocation || m_navigationLocation == -1 ? nullptr : m_forwardBackHistory.at(m_navigationLocation)->m_page ); }
    static PluginManager* getPluginManager() { return m_pluginManager; }
    void CloseDatabase();
    static void addMenuItem(QMenu* menu, const QString& submenu, const QString& menutitle, QAction* action, int section);

    QString mainConnectionName() const { return m_mainConnectionName; }

    QFontComboBox* fontComboBox() { return m_comboBoxFont; }
    QComboBox* fontSizeComboBox() { return m_comboBoxSize; }
    QComboBox* fontStyleComboBox() { return m_comboBoxStyle; }

public slots:
    void slotOpen_ProjectDetails_triggered(QVariant recordId);
    void slotOpen_ItemDetails_triggered(QVariant recordId);
    void slotOpen_ProjectNote_triggered(QVariant recordId);
    void slotOpen_SearchResults_triggered(QVariant recordId);
    void slotOpenTeamMember_triggered(QVariant recordId);
    void slotOpenLocation_triggered(QVariant recordId);
    void on_focusChanged(QWidget *old, QWidget *now);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void setButtonAndMenuStates();
    void openDatabase(const QString& dbfile);
    void on_actionExit_triggered();
    void on_actionOpen_Database_triggered();
    void on_actionClosed_Projects_triggered();
    void on_actionStatus_Bar_triggered();
    void on_actionFilter_triggered();
    void on_actionClients_triggered();
    void on_actionPeople_triggered();
    void on_actionProjects_triggered();
    void on_actionBack_triggered();
    void on_actionForward_triggered();
    void on_actionNew_Item_triggered();
    void on_actionCopy_Item_triggered();
    void on_actionDelete_Item_triggered();
    void on_actionInternal_Items_triggered();
    void on_actionPreferences_triggered();
    void on_actionResolved_Tracker_Action_Items_triggered();
    void on_actionAbout_triggered();
    void on_actionHelp_triggered();
    void on_actionWhat_s_New_triggered();

    void cursorPositionChanged();
    void alignmentChanged(Qt::Alignment a);
    void fontChanged(const QFont &f);
    void currentCharFormatChanged(const QTextCharFormat &format);

    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCopy_triggered();
    void on_actionCut_triggered();
    void on_actionPaste_triggered();
    void on_actionDelete_triggered();
    void on_actionSelect_All_triggered();
    void on_actionSpell_Check_triggered();

    void on_actionFind_triggered();
    void on_actionSearch_triggered();
    void on_pushButtonSearch_clicked();
    void on_actionView_LogView_triggered();
    void on_actionXML_Import_triggered();
    void on_actionXML_Export_triggered();
    void on_actionSync_All_triggered();

    void slotPluginMenu(Plugin* plugin, const QString& functionname, const QString& parameter);
    void on_actionOpen_Item_triggered();
    void on_actionIncrease_Font_Size_triggered();
    void on_actionDecrease_Font_Size_triggered();

    void onPluginLoaded(const QString& pluginpath);
    void onPluginUnLoaded(const QString& pluginpath);
    void onRefreshRequested();
    void onTimerWaitForThreads();
    void onSyncRowChanged(const QString& tableName, const QString& id);
    void onSyncStatusUpdated(int percentComplete, qint64 pendingPush, qint64 pendingPull);

private:
    void buildPluginMenu(BasePage* currentPage);
    void adjustFontSize(int delta);
    void dispatchEditAction(std::function<void(QTextEdit*)> textEditAction,
                            std::function<void(QPlainTextEdit*)> plainTextAction,
                            std::function<void(QLineEdit*)> lineEditAction);

    Ui::MainWindow *ui;

    PreferencesDialog* m_preferencesDialog = nullptr;
    FindReplaceDialog* m_findReplaceDialog = nullptr;

    static PluginManager* m_pluginManager;
    LogViewer* m_logviewDialog = nullptr;

    QStack<HistoryNode*> m_pageHistory;
    QStack<HistoryNode*> m_forwardBackHistory;
    int m_navigationLocation = -1;

    const QString rsrcPath = ":/icons";

    // setup complex text formatting toolbar and menu
    void setupTextActions();
    void textStyle(int styleIndex);
    void textFamily(const QString &f);
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void textSize(const QString &p);
    void modifyIndentation(int amount);
    void colorChanged(const QColor &c);
    void textItalic();
    void textBold();
    void textUnderline();
    void unindent();
    void indent();
    void textAlign(QAction *a);
    void textColor();

    QAction *m_actionTextBold;
    QAction *m_actionTextUnderline;
    QAction *m_actionTextItalic;
    QAction *m_actionTextColor;
    QAction *m_actionAlignLeft;
    QAction *m_actionAlignCenter;
    QAction *m_actionAlignRight;
    QAction *m_actionAlignJustify;
    QAction *m_actionIndentLess;
    QAction *m_actionIndentMore;
    QAction *m_actionUndo;
    QAction *m_actionRedo;
    QAction *m_actionCut;
    QAction *m_actionCopy;
    QAction *m_actionPaste;

    QComboBox* m_comboBoxStyle;
    QFontComboBox* m_comboBoxFont;
    QComboBox* m_comboBoxSize;

    QString m_mainConnectionName = "mainconnection";
    QTimer* m_waitForThreadsTimer = nullptr;

    SqliteSyncPro* m_syncApi = nullptr;
    QProgressBar* m_syncProgressBar = nullptr;
};


#endif // MAINWINDOW_H

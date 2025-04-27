// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "pncombobox.h"

#include <QMainWindow>
#include <QStringListModel>
#include <QStack>
#include <QTextCharFormat>
#include <QTimer>
#include <QComboBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include "pndatabaseobjects.h"
#include "pnsettings.h"
#include "pnbasepage.h"
#include "preferencesdialog.h"
#include "spellcheckdialog.h"
#include "findreplacedialog.h"
#include "logviewer.h"
#include "pluginmanager.h"

#define MAXHISTORYNODES 20

class HistoryNode
{
 public:
    void stamp() { m_timestamp = QDateTime::currentDateTime(); }
    bool equals(HistoryNode* node) {
        return node->m_page == m_page &&
               (node->m_pagetitle.compare(m_pagetitle) == 0) &&
               (node->m_record_id.toString().compare(m_record_id.toString()) == 0); }

    HistoryNode(PNBasePage* t_page, QVariant t_record_id, QString t_pagetitle) { m_page = t_page; m_record_id = t_record_id; m_pagetitle = t_pagetitle; stamp(); }
    PNBasePage* m_page;
    QVariant m_record_id;
    QString m_pagetitle;
    QDateTime m_timestamp;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *t_parent = nullptr);
    ~MainWindow();

    void navigateToPage(PNBasePage* t_widget, QVariant t_record_id);
    void navigateForward();
    void navigateBackward();
    bool navigateAtEnd() { return (m_navigation_location == (m_forward_back_history.count() - 1)); }
    bool navigateAtStart() { return (m_navigation_location <= 0); }
    void buildHistory(HistoryNode* t_node);
    void navigateClearHistory() { m_navigation_location = -1; m_forward_back_history.clear(); }
    PNBasePage* navigateCurrentPage() { return ((m_forward_back_history.count() - 1) < m_navigation_location || m_navigation_location == -1 ? nullptr : m_forward_back_history.at(m_navigation_location)->m_page ); }
    static PluginManager* getPluginManager() { return m_plugin_manager; }
    void CloseDatabase();
    static void addMenuItem(QMenu* t_menu, const QString& t_submenu, const QString& t_menutitle, QAction* t_action, int t_section);

    QString mainConnectionName() const { return m_main_connection_name; }

public slots:
    void slotOpen_ProjectDetails_triggered(QVariant t_record_id);
    void slotOpen_ItemDetails_triggered(QVariant t_record_id);
    void slotOpen_ProjectNote_triggered(QVariant t_record_id);
    void slotOpen_SearchResults_triggered(QVariant t_record_id);
    void slotOpenTeamMember_triggered(QVariant t_record_id);
    void slotOpenLocation_triggered(QVariant t_record_id);
    void on_focusChanged(QWidget *t_old, QWidget *t_now);

    void aboutToQuit();

private slots:
    void setButtonAndMenuStates();
    void openDatabase(QString t_dbfile);
    void on_actionExit_triggered();
    void on_actionNew_Database_triggered();
    void on_actionOpen_Database_triggered();
    void on_actionClose_Database_triggered();
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
    void on_actionBackup_Database_triggered();
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
    void on_lineEditSearchText_returnPressed();
    void on_actionView_LogView_triggered();
    void on_actionXML_Import_triggered();
    void on_actionXML_Export_triggered();

    void slotPluginMenu(Plugin* t_plugin, const QString& t_functionname, const QString& t_parameter);
    void on_actionOpen_Item_triggered();
    void on_actionIncrease_Font_Size_triggered();
    void on_actionDecrease_Font_Size_triggered();

    void onPluginLoaded(const QString& t_pluginpath);
    void onPluginUnLoaded(const QString& t_pluginpath);

private:
    void buildPluginMenu(PNBasePage* t_current_page);

    Ui::MainWindow *ui;

    PreferencesDialog* m_preferences_dialog = nullptr;
    FindReplaceDialog* m_find_replace_dialog = nullptr;
    static PluginManager* m_plugin_manager;
    LogViewer* m_logview_dialog = nullptr;

    QStack<HistoryNode*> m_page_history;
    QStack<HistoryNode*> m_forward_back_history;
    int m_navigation_location = -1;

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

    QComboBox* m_combo_box_style;
    QComboBox* m_combo_box_font;
    QComboBox* m_combo_box_size;

    QString m_main_connection_name = "mainconnection";
};


#endif // MAINWINDOW_H

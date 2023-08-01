#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QStack>
#include <QComboBox>
#include <QTextCharFormat>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include "pndatabaseobjects.h"
#include "pnsettings.h"
#include "pnbasepage.h"
#include "preferencesdialog.h"
#include "spellcheckdialog.h"
#include "findreplacedialog.h"
#include "pnpluginmanager.h"
#include "pluginsettingsdialog.h"

#define PNMajorVersion 3
#define PNMinorVersion 0
#define PNFixVersion 0

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *t_parent = nullptr);
    ~MainWindow();

    void navigateToPage(PNBasePage* t_widget);
    void navigateForward();
    void navigateBackward();
    bool navigateAtEnd() { return (m_navigation_location == (m_navigation_history.count() - 1)); }
    bool navigateAtStart() { return (m_navigation_location <= 0); }
    void navigateClearHistory() { m_navigation_location = -1; m_navigation_history.clear(); }
    PNBasePage* navigateCurrentPage() { return (m_navigation_location == -1 ? nullptr : m_navigation_history.at(m_navigation_location) ); }
    static PNPluginManager* getPluginManager() { return m_plugin_manager; }
    void buildPluginMenu();
    void CloseDatabase();

public slots:
    void on_actionOpen_ProjectDetails_triggered();
    void on_actionOpen_ItemDetails_triggered();
    void on_actionOpen_ProjectNote_triggered();
    void on_actionOpen_SearchResults_triggered();
    void on_actionOpenTeamMember_triggered();
    void on_focusChanged(QWidget *t_old, QWidget *t_now);

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
    void on_actionGetting_Started_triggered();
    void on_actionWhat_s_New_triggered();
    void on_actionCustom_Plugins_triggered();

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
    void on_actionPlugin_Settings_triggered();
    void on_actionView_Console_triggered();
    void on_actionXML_Import_triggered();
    void on_actionXML_Export_triggered();

    void slotPluginMenu(PNPlugin* t_plugin);
    void slotStartupEvent(PNPlugin* t_plugin);
    void slotShutdownEvent(PNPlugin* t_plugin);
    void slotTimerEvent(PNPlugin* t_plugin);
    void slotTimerUpdates();
    void on_actionOpen_Item_triggered();

private:
    Ui::MainWindow *ui;   

    PreferencesDialog* m_preferences_dialog = nullptr;
    SpellCheckDialog* m_spellcheck_dialog = nullptr;
    FindReplaceDialog* m_find_replace_dialog = nullptr;
    static PNPluginManager* m_plugin_manager;
    PluginSettingsDialog* m_plugin_settings_dialog = nullptr;
    QTimer* m_timer = nullptr;
    long m_minute_counter = 0;

    // view state
    QList<int> m_page_history;

    QStack<PNBasePage*> m_navigation_history;
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
};


#endif // MAINWINDOW_H

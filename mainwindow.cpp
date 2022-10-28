#include "ui_mainwindow.h"

#include "pntableview.h"
#include "projectslistmodel.h"
#include "pnsqlquerymodel.h"

#include <QStringListModel>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QTextEdit>
#include <QFontComboBox>
#include <QTextList>
#include <QColorDialog>

#include "mainwindow.h"


MainWindow::MainWindow(QWidget *t_parent)
    : QMainWindow(t_parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // add special formatting button for html editor
    setupTextActions();

    m_preferences_dialog = new PreferencesDialog(this);

    // view state
    m_page_history.clear();

    if (!global_Settings.getLastDatabase().toString().isEmpty())
        openDatabase(global_Settings.getLastDatabase().toString());

    setButtonAndMenuStates();

    global_Settings.getWindowState("MainWindow", *this);
    ui->actionStatus_Bar->setChecked(statusBar()->isVisibleTo(this));

    connect(ui->tableViewProjects, SIGNAL(signalOpenRecordWindow()), this, SLOT(on_actionOpen_ProjectDetails_triggered()));
    connect(ui->tableViewTrackerItems, SIGNAL(signalOpenRecordWindow()), this, SLOT(on_actionOpen_ItemDetails_triggered()));
    connect(ui->tableViewProjectNotes, SIGNAL(signalOpenRecordWindow()), this, SLOT(on_actionOpen_ProjectNote_triggered()));

    connect(ui->textEditNotes, &QTextEdit::currentCharFormatChanged, this, &MainWindow::currentCharFormatChanged);
    connect(ui->textEditNotes, &QTextEdit::cursorPositionChanged, this, &MainWindow::cursorPositionChanged);
}

MainWindow::~MainWindow()
{
    disconnect(ui->tableViewProjects, SIGNAL(signalOpenRecordWindow()), this, SLOT(on_actionOpen_ProjectDetails_triggered()));
    disconnect(ui->tableViewTrackerItems, SIGNAL(signalOpenRecordWindow()), this, SLOT(on_actionOpen_ItemDetails_triggered()));

    // need to save the screen layout befor the model is removed from the view
    // The destructor of PNTableview does not save the state
    ui->tableViewProjects->setModel(nullptr);
    ui->tableViewClients->setModel(nullptr);
    ui->tableViewPeople->setModel(nullptr);
    ui->tableViewStatusReportItems->setModel(nullptr);
    ui->tableViewTeam->setModel(nullptr);
    ui->tableViewTrackerItems->setModel(nullptr);
    ui->tableViewAtendees->setModel(nullptr);

    if (global_DBObjects.isOpen())
        global_DBObjects.closeDatabase();

    global_Settings.setWindowState("MainWindow", *this);

    delete m_preferences_dialog;

    delete ui;
}

/*
void MainWindow::handleDeleteProjectClicked()
{
    navigateCurrentPage()->deleteItem();


    QModelIndexList qi = ui->t_tableViewProjects->selectionModel()->selectedRows();

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        global_DBObjects.projectinformationmodel()->DeleteRecord(qi[i]);
    }

}
*/

void MainWindow::setButtonAndMenuStates()
{
    bool dbopen = global_DBObjects.isOpen();

    ui->stackedWidget->setVisible(dbopen);

    ui->actionSearch->setEnabled(dbopen);
    ui->actionXML_Export->setEnabled(dbopen);
    ui->actionXML_Import->setEnabled(dbopen);
    ui->actionBackup_Database->setEnabled(dbopen);

    /*
    QAction *actionFind;
    QAction *actionSpell_Check;
    QAction *actionUndo;
    QAction *actionRedo;
    QAction *actionCopy;
    QAction *actionCut;
    QAction *actionPaste;
    QAction *actionDelete;
    QAction *actionSelect_All;
    */


    ui->actionInternal_Items->setEnabled(dbopen);
    ui->actionAll_Tracker_Action_Items->setEnabled(dbopen);

    ui->actionProjects->setEnabled(dbopen);
    ui->actionClosed_Projects->setEnabled(dbopen);


    ui->actionNew_Item->setEnabled(dbopen);
    ui->actionCopy_Item->setEnabled(dbopen);
    ui->actionDelete_Item->setEnabled(dbopen);
    ui->actionEdit_Items->setEnabled(dbopen);
    ui->actionBack->setEnabled(!navigateAtStart());
    ui->actionForward->setEnabled(!navigateAtEnd());
    ui->actionClients->setEnabled(dbopen);
    ui->actionPeople->setEnabled(dbopen);
    ui->actionFilter->setEnabled(dbopen);

    if (dbopen)
    {
        ui->actionClosed_Projects->setChecked(global_DBObjects.getShowClosedProjects());
        ui->actionInternal_Items->setChecked(global_DBObjects.getShowInternalItems());

        if (global_DBObjects.getShowInternalItems())
        {
            ui->tableViewTrackerItems->setColumnHidden(15, false);
            ui->tableViewTrackerItems->resizeColumnToContents(15);
        }
        else
            ui->tableViewTrackerItems->setColumnHidden(15, true);

        ui->tabWidgetProject->setTabText(2, "Tracker");

        ui->tableViewTrackerItems->setColumnHidden(0, true);
        ui->tableViewTrackerItems->setColumnHidden(14, true);
        ui->tableViewTrackerItems->setColumnHidden(17, true);
        ui->tableViewTrackerItems->setColumnHidden(18, true);
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionOpen_Database_triggered()
{
    QString dbfile = QFileDialog::getOpenFileName(this, tr("Open Project Notes file"), QString(), tr("Project Notes (*.db)"));

    if (!dbfile.isEmpty())
    {
        openDatabase(dbfile);
    }
}

void MainWindow::openDatabase(QString t_dbfile)
{
    if (!global_DBObjects.openDatabase(t_dbfile))
        return;

    // load and refresh all of the models in order of their dependancy relationships
    global_DBObjects.unfilteredpeoplemodel()->refresh();
    global_DBObjects.unfilteredclientsmodel()->refresh();

    global_DBObjects.setGlobalSearches(false);

    global_DBObjects.clientsmodel()->loadUserFilter(global_DBObjects.clientsmodel()->objectName());
    global_DBObjects.clientsmodel()->activateUserFilter(global_DBObjects.clientsmodel()->objectName());

    global_DBObjects.peoplemodel()->loadUserFilter(global_DBObjects.peoplemodel()->objectName());
    global_DBObjects.peoplemodel()->activateUserFilter(global_DBObjects.peoplemodel()->objectName());

    global_DBObjects.projectslistmodel()->loadUserFilter(global_DBObjects.projectslistmodel()->objectName());
    global_DBObjects.projectslistmodel()->activateUserFilter(global_DBObjects.projectslistmodel()->objectName());

    global_Settings.setLastDatabase(t_dbfile);

    // assign all of the newly open models
    ui->pageProjectsList->setupModels(ui);
    ui->pageClients->setupModels(ui);
    ui->pagePeople->setupModels(ui);
    ui->pageProjectDetails->setupModels(ui);
    ui->pageItemDetails->setupModels(ui);
    ui->pageProjectNote->setupModels(ui);

    navigateClearHistory();
    navigateToPage(ui->pageProjectsList);
    //navigateToPage(ui->pageClients);

    setButtonAndMenuStates();
}

void MainWindow::navigateToPage(PNBasePage* t_widget)
{
    if ( t_widget == navigateCurrentPage() )
        return;

    // if in the middle of the history chop off the remaining history
    while (m_navigation_location < m_navigation_history.count() - 1)
        m_navigation_history.pop();

    m_navigation_location = m_navigation_history.count();
    m_navigation_history.push(t_widget);

    ui->stackedWidget->setCurrentWidget(t_widget);

    this->setWindowTitle(QString("Project Notes [%1]").arg(t_widget->pagetitle()));

    setButtonAndMenuStates();
}

void MainWindow::navigateBackward()
{
    if (m_navigation_location > 0)
    {
        m_navigation_location--;

        QWidget* current = m_navigation_history.at(m_navigation_location);
        ui->stackedWidget->setCurrentWidget(current);

        this->setWindowTitle(QString("Project Notes [%1]").arg(((PNBasePage* )current)->pagetitle()));
    }

    setButtonAndMenuStates();
}

void MainWindow::navigateForward()
{
    if (m_navigation_location < (m_navigation_history.count() - 1) )
    {
        m_navigation_location++;

        QWidget* current = m_navigation_history.at(m_navigation_location);
        ui->stackedWidget->setCurrentWidget(current);

        this->setWindowTitle(QString("Project Notes [%1]").arg(((PNBasePage* )current)->pagetitle()));
    }

    setButtonAndMenuStates();
}

void MainWindow::on_actionClose_Database_triggered()
{
    global_Settings.setLastDatabase(QString());
    global_DBObjects.closeDatabase();
    setButtonAndMenuStates();
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
    PNTableView* curview = navigateCurrentPage()->getCurrentView();

    curview->filterDialog();
}

void MainWindow::on_actionClients_triggered()
{
    navigateToPage(ui->pageClients);
}

void MainWindow::on_actionPeople_triggered()
{
    navigateToPage(ui->pagePeople);
}

void MainWindow::on_actionProjects_triggered()
{
    navigateToPage(ui->pageProjectsList);
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

void MainWindow::on_actionCopy_Item_triggered()
{
    if ( navigateCurrentPage() )
        navigateCurrentPage()->copyItem();
}

void MainWindow::on_actionDelete_Item_triggered()
{
    if ( navigateCurrentPage() )
        navigateCurrentPage()->deleteItem();
}

void MainWindow::on_actionOpen_ProjectDetails_triggered()
{

    ui->pageProjectDetails->toFirst();

    navigateToPage(ui->pageProjectDetails);
}

void MainWindow::on_actionOpen_ItemDetails_triggered()
{

    ui->pageItemDetails->toFirst();

    navigateToPage(ui->pageItemDetails);
}

void MainWindow::on_actionOpen_ProjectNote_triggered()
{

    ui->pageProjectNote->toFirst();

    navigateToPage(ui->pageProjectNote);
}
void MainWindow::on_actionInternal_Items_triggered()
{
    global_DBObjects.setShowInternalItems(ui->actionInternal_Items->isChecked());
    global_DBObjects.setGlobalSearches(true);

    setButtonAndMenuStates();
}

void MainWindow::on_actionResolved_Tracker_Action_Items_triggered()
{
    global_DBObjects.setShowResolvedTrackerItems(ui->actionResolved_Tracker_Action_Items->isChecked());

    // filter tracker items by Resolved
    if (ui->actionResolved_Tracker_Action_Items->isChecked())
        global_DBObjects.trackeritemsmodel()->clearFilter(9);
    else
    {
        global_DBObjects.trackeritemsmodel()->setFilter(9, "Resolved", PNSqlQueryModel::NotEqual );
    }

    global_DBObjects.trackeritemsmodel()->refresh();

    setButtonAndMenuStates();
}

void MainWindow::on_actionPreferences_triggered()
{
    m_preferences_dialog->show();
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
    const QString rsrcPath = ":/icons";

    QToolBar *tb = ui->toolBarFormat;
    QMenu *menu = ui->menuFormat;

    const QIcon boldIcon = QIcon::fromTheme("format-text-bold", QIcon(rsrcPath + "/textbold.png"));
    m_actionTextBold = menu->addAction(boldIcon, tr("&Bold"), this, &MainWindow::textBold);
    m_actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    m_actionTextBold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    m_actionTextBold->setFont(bold);
    tb->addAction(m_actionTextBold);
    m_actionTextBold->setCheckable(true);

    const QIcon italicIcon = QIcon::fromTheme("format-text-italic", QIcon(rsrcPath + "/textitalic.png"));
    m_actionTextItalic = menu->addAction(italicIcon, tr("&Italic"), this, &MainWindow::textItalic);
    m_actionTextItalic->setPriority(QAction::LowPriority);
    m_actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    m_actionTextItalic->setFont(italic);
    tb->addAction(m_actionTextItalic);
    m_actionTextItalic->setCheckable(true);

    const QIcon underlineIcon = QIcon::fromTheme("format-text-underline", QIcon(rsrcPath + "/textunder.png"));
    m_actionTextUnderline = menu->addAction(underlineIcon, tr("&Underline"), this, &MainWindow::textUnderline);
    m_actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    m_actionTextUnderline->setPriority(QAction::LowPriority);
    QFont underline;
    underline.setUnderline(true);
    m_actionTextUnderline->setFont(underline);
    tb->addAction(m_actionTextUnderline);
    m_actionTextUnderline->setCheckable(true);

    menu->addSeparator();

    const QIcon leftIcon = QIcon::fromTheme("format-justify-left", QIcon(rsrcPath + "/textleft.png"));
    m_actionAlignLeft = new QAction(leftIcon, tr("&Left"), this);
    m_actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    m_actionAlignLeft->setCheckable(true);
    m_actionAlignLeft->setPriority(QAction::LowPriority);
    const QIcon centerIcon = QIcon::fromTheme("format-justify-center", QIcon(rsrcPath + "/textcenter.png"));
    m_actionAlignCenter = new QAction(centerIcon, tr("C&enter"), this);
    m_actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    m_actionAlignCenter->setCheckable(true);
    m_actionAlignCenter->setPriority(QAction::LowPriority);
    const QIcon rightIcon = QIcon::fromTheme("format-justify-right", QIcon(rsrcPath + "/textright.png"));
    m_actionAlignRight = new QAction(rightIcon, tr("&Right"), this);
    m_actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    m_actionAlignRight->setCheckable(true);
    m_actionAlignRight->setPriority(QAction::LowPriority);
    const QIcon fillIcon = QIcon::fromTheme("format-justify-fill", QIcon(rsrcPath + "/textjustify.png"));
    m_actionAlignJustify = new QAction(fillIcon, tr("&Justify"), this);
    m_actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    m_actionAlignJustify->setCheckable(true);
    m_actionAlignJustify->setPriority(QAction::LowPriority);
    const QIcon indentMoreIcon = QIcon::fromTheme("format-indent-more", QIcon(rsrcPath + "/format-indent-more.png"));
    m_actionIndentMore = menu->addAction(indentMoreIcon, tr("&Indent"), this, &MainWindow::indent);
    m_actionIndentMore->setShortcut(Qt::CTRL + Qt::Key_BracketRight);
    m_actionIndentMore->setPriority(QAction::LowPriority);
    const QIcon indentLessIcon = QIcon::fromTheme("format-indent-less", QIcon(rsrcPath + "/format-indent-less.png"));
    m_actionIndentLess = menu->addAction(indentLessIcon, tr("&Unindent"), this, &MainWindow::unindent);
    m_actionIndentLess->setShortcut(Qt::CTRL + Qt::Key_BracketLeft);
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
    tb->addAction(m_actionTextColor);

    menu->addSeparator();

    tb->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    m_combo_box_style = new QComboBox(tb);
    tb->addWidget(m_combo_box_style);
    m_combo_box_style->addItem("Standard");
    m_combo_box_style->addItem("Bullet List (Disc)");
    m_combo_box_style->addItem("Bullet List (Circle)");
    m_combo_box_style->addItem("Bullet List (Square)");
    m_combo_box_style->addItem("Task List (Unchecked)");
    m_combo_box_style->addItem("Task List (Checked)");
    m_combo_box_style->addItem("Ordered List (Decimal)");
    m_combo_box_style->addItem("Ordered List (Alpha lower)");
    m_combo_box_style->addItem("Ordered List (Alpha upper)");
    m_combo_box_style->addItem("Ordered List (Roman lower)");
    m_combo_box_style->addItem("Ordered List (Roman upper)");
    m_combo_box_style->addItem("Heading 1");
    m_combo_box_style->addItem("Heading 2");
    m_combo_box_style->addItem("Heading 3");
    m_combo_box_style->addItem("Heading 4");
    m_combo_box_style->addItem("Heading 5");
    m_combo_box_style->addItem("Heading 6");

    connect(m_combo_box_style, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::textStyle);

    m_combo_box_font = new QFontComboBox(tb);
    tb->addWidget(m_combo_box_font);
    connect(m_combo_box_font, &QComboBox::textActivated, this, &MainWindow::textFamily);

    m_combo_box_size = new QComboBox(tb);
    m_combo_box_size->setObjectName("comboSize");
    tb->addWidget(m_combo_box_size);
    m_combo_box_size->setEditable(true);

    const QList<int> standardSizes = QFontDatabase::standardSizes();
    for (int size : standardSizes)
        m_combo_box_size->addItem(QString::number(size));
    m_combo_box_size->setCurrentIndex(standardSizes.indexOf(QApplication::font().pointSize()));

    connect(m_combo_box_size, &QComboBox::textActivated, this, &MainWindow::textSize);
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
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui->textEditNotes->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui->textEditNotes->mergeCurrentCharFormat(format);
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
            m_combo_box_style->setCurrentIndex(1);
            break;
        case QTextListFormat::ListCircle:
            m_combo_box_style->setCurrentIndex(2);
            break;
        case QTextListFormat::ListSquare:
            m_combo_box_style->setCurrentIndex(3);
            break;
        case QTextListFormat::ListDecimal:
            m_combo_box_style->setCurrentIndex(6);
            break;
        case QTextListFormat::ListLowerAlpha:
            m_combo_box_style->setCurrentIndex(7);
            break;
        case QTextListFormat::ListUpperAlpha:
            m_combo_box_style->setCurrentIndex(8);
            break;
        case QTextListFormat::ListLowerRoman:
            m_combo_box_style->setCurrentIndex(9);
            break;
        case QTextListFormat::ListUpperRoman:
            m_combo_box_style->setCurrentIndex(10);
            break;
        default:
            m_combo_box_style->setCurrentIndex(-1);
            break;
        }
/*
        switch (ui->textEditNotes->textCursor().block().blockFormat().marker())
        {
        case QTextBlockFormat::MarkerType::NoMarker:
            m_actionToggleCheckState->setChecked(false);
            break;
        case QTextBlockFormat::MarkerType::Unchecked:
            m_combo_box_style->setCurrentIndex(4);
            m_actionToggleCheckState->setChecked(false);
            break;
        case QTextBlockFormat::MarkerType::Checked:
            m_combo_box_style->setCurrentIndex(5);
            m_actionToggleCheckState->setChecked(true);
            break;
        }*/
    }
    else
    {
        int headingLevel = ui->textEditNotes->textCursor().blockFormat().headingLevel();
        m_combo_box_style->setCurrentIndex(headingLevel ? headingLevel + 10 : 0);
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
    m_combo_box_font->setCurrentIndex(m_combo_box_font->findText(QFontInfo(f).family()));
    m_combo_box_size->setCurrentIndex(m_combo_box_size->findText(QString::number(f.pointSize())));
    m_actionTextBold->setChecked(f.bold());
    m_actionTextItalic->setChecked(f.italic());
    m_actionTextUnderline->setChecked(f.underline());
}

// STOPPED HERE - Need to add cut copy past functions
// TODO: Add spell checking features

#include <QDebug>
#include <QUrl>
#include <QHelpSearchEngine>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#include <QHelpContentWidget>

#include "helppage.h"
#include "ui_mainwindow.h"

HelpPage::HelpPage()
{

}

void HelpPage::setPageTitle()
{
    topLevelWidget()->setWindowTitle(QString("Project Notes Help"));
}

void HelpPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (!t_ui)
    {
        disconnect((QWidget*)ui->textBrowser->helpEngine()->contentWidget(),
                SIGNAL(linkActivated(QUrl)),
                ui->textBrowser, SLOT(setSource(QUrl)));

        disconnect((QWidget*)ui->textBrowser->helpEngine()->indexWidget(),
                SIGNAL(linkActivated(QUrl, QString)),
                ui->textBrowser, SLOT(setSource(QUrl)));

        return; // closing application
    }

    setCurrentModel( nullptr );
    setCurrentView( nullptr );


    if (ui->textBrowser->helpEngine())
    {
        ui->tabWidgetContents->addTab((QWidget*)ui->textBrowser->helpEngine()->contentWidget(), "Contents");
        ui->tabWidgetContents->addTab((QWidget*)ui->textBrowser->helpEngine()->indexWidget(), "Index");

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget((QWidget*)ui->textBrowser->helpEngine()->searchEngine()->queryWidget());
        layout->addWidget((QWidget*)ui->textBrowser->helpEngine()->searchEngine()->resultWidget());
        QWidget* widget = new QWidget();
        widget->setLayout(layout);

        ui->tabWidgetContents->addTab(widget, "Search");
        ui->tabWidgetContents->removeTab(0);
        ui->tabWidgetContents->removeTab(0);

        ui->textBrowser->helpEngine()->searchEngine()->reindexDocumentation();
    }

    connect((QWidget*)ui->textBrowser->helpEngine()->contentWidget(),
            SIGNAL(linkActivated(QUrl)),
            ui->textBrowser, SLOT(setSource(QUrl)));

    connect((QWidget*)ui->textBrowser->helpEngine()->indexWidget(),
            SIGNAL(linkActivated(QUrl, QString)),
            ui->textBrowser, SLOT(setSource(QUrl)));

    connect(ui->textBrowser->helpEngine()->searchEngine()->queryWidget(), &QHelpSearchQueryWidget::search,
            this, &HelpPage::search);

    connect(ui->textBrowser->helpEngine()->searchEngine()->resultWidget(), &QHelpSearchResultWidget::requestShowLink,
            this,  &HelpPage::showLink);
}

void HelpPage::search()
{
    ui->textBrowser->helpEngine()->searchEngine()->search(ui->textBrowser->helpEngine()->searchEngine()->queryWidget()->searchInput());
}

void HelpPage::showLink(const QUrl &url)
{
    ui->textBrowser->setSource(url);
}

void HelpPage::setButtonAndMenuStates()
{

}

// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "basepage.h"
#include "databaseobjects.h"
#include "databaseobjects.h"
#include "pythonworker.h"
#include "plugin.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QApplication>
#include <QScrollBar>
#include <QAbstractItemModel>
#include <QTextCursor>

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

BasePage::BasePage(QWidget *parent) : QWidget(parent)
{

}

void BasePage::setPageTitle()
{

}

void BasePage::saveState()
{
    QDomDocument doc;
    QDomElement root = doc.createElement("PageState");
    doc.appendChild(root);

    QDomElement child;

    QList<QWidget*> childwidgets = findChildren<QWidget*>();

    if (focusWidget())
    {
        root.setAttribute("FocusWidget", focusWidget()->objectName());
    }

    for (QWidget* widget : childwidgets)
    {
        if ( QString("TableView").compare( widget->metaObject()->className() ) == 0)
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            if (qobject_cast<TableView*>(widget)->selectionModel()->hasSelection())
                child.setAttribute("SelectedRow", QString("%1").arg(qobject_cast<TableView*>(widget)->selectionModel()->selectedRows().at(0).row()));
            child.setAttribute("VerticalScroll", QString("%1").arg(qobject_cast<TableView*>(widget)->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString("%1").arg(qobject_cast<TableView*>(widget)->horizontalScrollBar()->value()));

            root.appendChild(child);
        }
        else if ( QString("PlainTextEdit").compare( widget->metaObject()->className() ) == 0)
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("VerticalScroll", QString("%1").arg(qobject_cast<PlainTextEdit*>(widget)->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString("%1").arg(qobject_cast<PlainTextEdit*>(widget)->horizontalScrollBar()->value()));
            child.setAttribute("SelectionStart", QString("%1").arg(qobject_cast<PlainTextEdit*>(widget)->textCursor().selectionStart()));
            child.setAttribute("SelectionEnd", QString("%1").arg(qobject_cast<PlainTextEdit*>(widget)->textCursor().selectionEnd()));
            child.setAttribute("CursorPosition", QString("%1").arg(qobject_cast<PlainTextEdit*>(widget)->textCursor().position()));

            root.appendChild(child);
        }
        else if ( QString("QTabWidget").compare( widget->metaObject()->className() ) == 0)
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("CurrentIndex", QString("%1").arg(qobject_cast<QTabWidget*>(widget)->currentIndex()));

            root.appendChild(child);
        }
        else if ( QString("TextEdit").compare( widget->metaObject()->className() ) == 0)
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("VerticalScroll", QString("%1").arg(qobject_cast<TextEdit*>(widget)->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString("%1").arg(qobject_cast<TextEdit*>(widget)->horizontalScrollBar()->value()));
            child.setAttribute("SelectionStart", QString("%1").arg(qobject_cast<TextEdit*>(widget)->textCursor().selectionStart()));
            child.setAttribute("SelectionEnd", QString("%1").arg(qobject_cast<TextEdit*>(widget)->textCursor().selectionEnd()));
            child.setAttribute("CursorPosition", QString("%1").arg(qobject_cast<TextEdit*>(widget)->textCursor().position()));

            root.appendChild(child);
        }

    }

    // Write the output to a QString.
    QString xml = doc.toString();

    QString parmname = QString("PageState:%1:%2").arg(objectName(), m_recordId.toString());

    global_DBObjects.saveParameter( parmname, xml );
}

void BasePage::loadState()
{
    QString parmname;

    parmname = QString("PageState:%1:%2").arg(objectName(), m_recordId.toString());

    QString xml = global_DBObjects.loadParameter(parmname);

    if (xml.isEmpty())
        return;

    QDomDocument doc;
    doc.setContent(xml);

    QDomElement root = doc.documentElement();
    QDomNode child;

    if (!root.isNull())
    {
        child = root.firstChild();

        while (!child.isNull())
        {
            QWidget* widget = findChild<QWidget*>(child.toElement().attribute("Name"));

            if (widget)
            {
                QString classname = widget->metaObject()->className();

                if ( QString("TableView").compare( classname ) == 0)
                {
                    int selectedrow = child.toElement().attribute("SelectedRow", "-1").toInt();
                    if (selectedrow > -1)
                    {
                        QAbstractItemModel* model = qobject_cast<TableView*>(widget)->model();

                        if (model)
                            qobject_cast<TableView*>(widget)->selectionModel()->select(model->index(selectedrow, 0),  QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    }

                    int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                    int hscroll = child.toElement().attribute("HorizontalScroll").toInt();


                    qobject_cast<TableView*>(widget)->updateGeometry();
                    qobject_cast<TableView*>(widget)->update();

                    // qDebug() << "Before setValue: value =" << qobject_cast<TableView*>(widget)->verticalScrollBar()->value()
                    //          << "min =" << qobject_cast<TableView*>(widget)->verticalScrollBar()->minimum()
                    //          << "max =" << qobject_cast<TableView*>(widget)->verticalScrollBar()->maximum()
                    //          << "pageStep =" << qobject_cast<TableView*>(widget)->verticalScrollBar()->pageStep()
                    //          << " set value used was " << vscroll << " on " << widget->objectName();

                    qobject_cast<TableView*>(widget)->verticalScrollBar()->setValue(vscroll);
                    qobject_cast<TableView*>(widget)->horizontalScrollBar()->setValue(hscroll);

                    qobject_cast<TableView*>(widget)->verticalScrollBar()->update();
                    qobject_cast<TableView*>(widget)->horizontalScrollBar()->update();

                    // // After setValue(...)
                    // qDebug() << "After setValue: actual value now =" << qobject_cast<TableView*>(widget)->verticalScrollBar()->value();
                    // if (vscroll != qobject_cast<TableView*>(widget)->verticalScrollBar()->value())
                    //     qDebug() << "SET FAILED!!!";
                }
                else if ( QString("PlainTextEdit").compare( widget->metaObject()->className() ) == 0)
                {
                    int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                    int hscroll = child.toElement().attribute("HorizontalScroll").toInt();

                    qobject_cast<PlainTextEdit*>(widget)->verticalScrollBar()->setValue(vscroll);
                    qobject_cast<PlainTextEdit*>(widget)->horizontalScrollBar()->setValue(hscroll);

                    int selectionstart = child.toElement().attribute("SelectionStart","0").toInt();
                    int selectionend = child.toElement().attribute("SelectionEnd","0").toInt();
                    int cursorposition = child.toElement().attribute("CurosrPosition","0").toInt();

                    QTextCursor qt = qobject_cast<PlainTextEdit*>(widget)->textCursor();

                    qt.setPosition(cursorposition);
                    qt.setPosition(selectionstart, QTextCursor::MoveAnchor);
                    qt.setPosition(selectionend, QTextCursor::KeepAnchor);

                    qobject_cast<PlainTextEdit*>(widget)->setTextCursor(qt);
                }
                else if ( QString("QTabWidget").compare( widget->metaObject()->className() ) == 0)
                {
                    qobject_cast<QTabWidget*>(widget)->setCurrentIndex(child.toElement().attribute("CurrentIndex","0").toInt());
                }
                else if ( QString("TextEdit").compare( widget->metaObject()->className() ) == 0)
                {
                    int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                    int hscroll = child.toElement().attribute("HorizontalScroll").toInt();

                    qobject_cast<TextEdit*>(widget)->verticalScrollBar()->setValue(vscroll);
                    qobject_cast<TextEdit*>(widget)->horizontalScrollBar()->setValue(hscroll);

                    int selectionstart = child.toElement().attribute("SelectionStart","0").toInt();
                    int selectionend = child.toElement().attribute("SelectionEnd","0").toInt();
                    int cursorposition = child.toElement().attribute("CurosrPosition","0").toInt();

                    QTextCursor qt = qobject_cast<TextEdit*>(widget)->textCursor();

                    qt.setPosition(cursorposition);
                    qt.setPosition(selectionstart, QTextCursor::MoveAnchor);
                    qt.setPosition(selectionend, QTextCursor::KeepAnchor);

                    qobject_cast<TextEdit*>(widget)->setTextCursor(qt);
                }
            }

            child = child.nextSibling();
        }

        QWidget* focus = findChild<QWidget*>(root.toElement().attribute("FocusWidget"));
        if (focus)
            focus->setFocus();
    }
}

void BasePage::openRecord(QVariant& recordId)
{
    m_recordId = recordId;

    // loadState();
}

void BasePage::newRecord()
{
    QModelIndex index = dynamic_cast<SqlQueryModel*>(getCurrentModel()->sourceModel())->newRecord();

    //  check if column is visible
    int col = 1;
    while (getCurrentView()->isColumnHidden(col))
        col++;

    QModelIndex sort_index = dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->index(
        dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->mapFromSource(index).row(), col);

    getCurrentView()->selectionModel()->select(sort_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    getCurrentView()->scrollTo(sort_index, QAbstractItemView::PositionAtCenter);
}

void BasePage::deleteItem()
{
    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();
    int v = getCurrentView()->verticalScrollBar()->value();
    int h = getCurrentView()->horizontalScrollBar()->value();

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        dynamic_cast<SqlQueryModel*>(getCurrentModel()->sourceModel())->deleteRecord(getCurrentModel()->mapToSource(qi[i]));

        // reposition the row select
        if (qi[i].row() > getCurrentModel()->rowCount(getCurrentView()->rootIndex()))
        {
            getCurrentView()->setCurrentIndex(getCurrentModel()->index(qi[i].row() - 1, qi[i].column()));
        }
        else
        {
            getCurrentView()->setCurrentIndex(qi[i]);
        }

        getCurrentView()->verticalScrollBar()->setValue(v);
        getCurrentView()->horizontalScrollBar()->setValue(h);
    }
}

void BasePage::copyItem()
{
    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();
    QModelIndex index;

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        index = dynamic_cast<SqlQueryModel*>(getCurrentModel()->sourceModel())->copyRecord(getCurrentModel()->mapToSource(qi[i]));
    }

    //  check if column is visible
    int col = 1;
    while (getCurrentView()->isColumnHidden(col))
        col++;

    QModelIndex sort_index = dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->index(
        dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->mapFromSource(index).row(), col);

    getCurrentView()->selectionModel()->select(sort_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    getCurrentView()->scrollTo(sort_index, QAbstractItemView::PositionAtCenter);
}

void BasePage::openItem()
{
    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();

    if (qi.count() > 0)
        getCurrentView()->slotOpenRecord();
}

void BasePage::setButtonAndMenuStates()
{

}

void BasePage::buildPluginMenu(PluginManager* pm, QMenu* menu)
{
    if (!m_currentModel)
        return; // no recordset to associate menu items with

    menu->addSeparator();

    // add menus relevant to the current table
    for ( Plugin* p : pm->plugins())
    {
        for ( PluginMenu m : p->pythonplugin().menus())
        {
            QString table = m_pageModel->tablename();
            if (m.dataexport().compare(table, Qt::CaseInsensitive) == 0) // only show right-click data menus
            {               
                QAction* act = new QAction(QIcon(":/icons/add-on.png"), m.menutitle(), this);
                connect(act, &QAction::triggered, this,[p, m, this](){slotPluginMenu(p, m.functionname(), m.tablefilter(), m.parameter());});
                MainWindow::addMenuItem(menu, m.submenu(), m.menutitle(), act, 2);
            }
        }
    }
}

void BasePage::slotPluginMenu(Plugin* plugin, const QString& functionname, const QString& tablefilter, const QString& parameter)
{
    QString response;

    if (m_pageModel)
    {
        submitRecord(); // make sure the current record is saved

        QVariant keyval;
        keyval = m_pageModel->data(m_pageModel->index(0, 0));

        SqlQueryModel *exportmodel = global_DBObjects.createExportObject(m_pageModel->tablename());
        exportmodel->setFilter(0, keyval.toString());
        exportmodel->refresh();

        QDomDocument* xdoc = global_DBObjects.createXMLExportDoc(exportmodel, tablefilter);
        QString xmlstr = xdoc->toString();

        //call the menu plugin with the data structure
        plugin->callXmlMethod(functionname, xmlstr, parameter);

        delete xdoc;
    }
}


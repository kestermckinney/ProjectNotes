// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
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
        const QLatin1StringView cn(widget->metaObject()->className());

        if (cn == "TableView")
        {
            auto* tv = qobject_cast<TableView*>(widget);
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            if (tv->selectionModel()->hasSelection())
                child.setAttribute("SelectedRow", QString::number(tv->selectionModel()->selectedRows().at(0).row()));
            child.setAttribute("VerticalScroll", QString::number(tv->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString::number(tv->horizontalScrollBar()->value()));

            root.appendChild(child);
        }
        else if (cn == "PlainTextEdit")
        {
            auto* pte = qobject_cast<PlainTextEdit*>(widget);
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("VerticalScroll", QString::number(pte->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString::number(pte->horizontalScrollBar()->value()));
            child.setAttribute("SelectionStart", QString::number(pte->textCursor().selectionStart()));
            child.setAttribute("SelectionEnd", QString::number(pte->textCursor().selectionEnd()));
            child.setAttribute("CursorPosition", QString::number(pte->textCursor().position()));

            root.appendChild(child);
        }
        else if (cn == "QTabWidget")
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("CurrentIndex", QString::number(qobject_cast<QTabWidget*>(widget)->currentIndex()));

            root.appendChild(child);
        }
        else if (cn == "TextEdit")
        {
            auto* te = qobject_cast<TextEdit*>(widget);
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("VerticalScroll", QString::number(te->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString::number(te->horizontalScrollBar()->value()));
            child.setAttribute("SelectionStart", QString::number(te->textCursor().selectionStart()));
            child.setAttribute("SelectionEnd", QString::number(te->textCursor().selectionEnd()));
            child.setAttribute("CursorPosition", QString::number(te->textCursor().position()));

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
                const QLatin1StringView classname(widget->metaObject()->className());

                if (classname == "TableView")
                {
                    auto* tv = qobject_cast<TableView*>(widget);
                    int selectedrow = child.toElement().attribute("SelectedRow", "-1").toInt();
                    if (selectedrow > -1)
                    {
                        QAbstractItemModel* model = tv->model();

                        if (model)
                            tv->selectionModel()->select(model->index(selectedrow, 0),  QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    }

                    int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                    int hscroll = child.toElement().attribute("HorizontalScroll").toInt();


                    tv->updateGeometry();
                    tv->update();

                    tv->verticalScrollBar()->setValue(vscroll);
                    tv->horizontalScrollBar()->setValue(hscroll);

                    tv->verticalScrollBar()->update();
                    tv->horizontalScrollBar()->update();
                }
                else if (classname == "PlainTextEdit")
                {
                    auto* pte = qobject_cast<PlainTextEdit*>(widget);
                    int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                    int hscroll = child.toElement().attribute("HorizontalScroll").toInt();

                    pte->verticalScrollBar()->setValue(vscroll);
                    pte->horizontalScrollBar()->setValue(hscroll);

                    int selectionstart = child.toElement().attribute("SelectionStart","0").toInt();
                    int selectionend = child.toElement().attribute("SelectionEnd","0").toInt();
                    int cursorposition = child.toElement().attribute("CurosrPosition","0").toInt();

                    QTextCursor qt = pte->textCursor();

                    qt.setPosition(cursorposition);
                    qt.setPosition(selectionstart, QTextCursor::MoveAnchor);
                    qt.setPosition(selectionend, QTextCursor::KeepAnchor);

                    pte->setTextCursor(qt);
                }
                else if (classname == "QTabWidget")
                {
                    qobject_cast<QTabWidget*>(widget)->setCurrentIndex(child.toElement().attribute("CurrentIndex","0").toInt());
                }
                else if (classname == "TextEdit")
                {
                    auto* te = qobject_cast<TextEdit*>(widget);
                    int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                    int hscroll = child.toElement().attribute("HorizontalScroll").toInt();

                    te->verticalScrollBar()->setValue(vscroll);
                    te->horizontalScrollBar()->setValue(hscroll);

                    int selectionstart = child.toElement().attribute("SelectionStart","0").toInt();
                    int selectionend = child.toElement().attribute("SelectionEnd","0").toInt();
                    int cursorposition = child.toElement().attribute("CurosrPosition","0").toInt();

                    QTextCursor qt = te->textCursor();

                    qt.setPosition(cursorposition);
                    qt.setPosition(selectionstart, QTextCursor::MoveAnchor);
                    qt.setPosition(selectionend, QTextCursor::KeepAnchor);

                    te->setTextCursor(qt);
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

    auto* proxy = dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model());
    QModelIndex sort_index = proxy->index(proxy->mapFromSource(index).row(), col);

    getCurrentView()->selectionModel()->select(sort_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    getCurrentView()->scrollTo(sort_index, QAbstractItemView::PositionAtCenter);
}

QVariantList BasePage::getSelectedRecordIds()
{
    QVariantList ids;
    if (!getCurrentView() || !getCurrentModel())
        return ids;

    SqlQueryModel* sourceModel = dynamic_cast<SqlQueryModel*>(getCurrentModel()->sourceModel());
    if (!sourceModel)
        return ids;

    for (const QModelIndex& proxyIdx : getCurrentView()->selectionModel()->selectedRows())
    {
        QModelIndex sourceIdx = getCurrentModel()->mapToSource(proxyIdx);
        QVariant id = sourceModel->data(sourceModel->index(sourceIdx.row(), 0));
        if (!id.isNull())
            ids.append(id);
    }
    return ids;
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

    auto* proxy = dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model());
    QModelIndex sort_index = proxy->index(proxy->mapFromSource(index).row(), col);

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
        for ( const PluginMenu& m : p->pythonplugin().menus())
        {
            QString table = m_pageModel->tablename();
            if (m.dataexport().compare(table, Qt::CaseInsensitive) == 0) // only show right-click data menus
            {               
                QAction* act = new QAction(QIcon(":/icons/add-on.png"), m.menutitle(), menu);
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


// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "helpbrowser.h"
#include <QCoreApplication>
#include <QLibraryInfo>
//#include <QDebug>

HelpBrowser::HelpBrowser(QWidget* parent):QTextBrowser(parent)
{
    m_helpengine = new QHelpEngine(QCoreApplication::applicationDirPath() +
                                   "/docs/Project Notes.qhc", parent);

    setOpenExternalLinks(false); //TODO: Fix this is broken in 6.7.2  I should be set to True to open a browser
}

HelpBrowser::~HelpBrowser()
{
    delete m_helpengine;
}

QVariant HelpBrowser::loadResource(int type, const QUrl &name)
{
    if (name.scheme() == "qthelp")
        return QVariant(m_helpengine->fileData(name));
    else
        return QTextBrowser::loadResource(type, name);
}

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

    m_helpengine->setupData();
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

// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "lineeditfilebutton.h"

#include <QPushButton>
#include <QWidget>
#include <QStyle>
#include <QResizeEvent>
#include <QHBoxLayout>
#include <QFileDialog>

LineEditFileButton::LineEditFileButton(QWidget *parent ) : QWidget( parent )
{
    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_lineEdit = new QLineEdit( this );
    layout->addWidget( m_lineEdit );

    connect( m_lineEdit, SIGNAL( textChanged( const QString & ) ),
             this, SIGNAL( fileNameChanged( const QString & ) ) );

    m_pushButton = new QPushButton( "...", this );
    m_pushButton->setFixedWidth( this->height() );

    layout->addWidget( m_pushButton );

    connect( m_pushButton, SIGNAL( clicked() ),
             this, SLOT( chooseFile() ) );

    setFocusProxy( m_lineEdit );
}

LineEditFileButton::~LineEditFileButton()
{

}

void LineEditFileButton::setFileName( const QString &fn )
{
    m_lineEdit->setText( fn );
}

QString LineEditFileButton::fileName() const
{
    return m_lineEdit->text();
}

void LineEditFileButton::chooseFile()
{
    QString fn;

    fn = QFileDialog::getOpenFileName( this );

    if ( !fn.isEmpty() ) {
        m_lineEdit->setText( fn );
        emit fileNameChanged( fn );
    }
}

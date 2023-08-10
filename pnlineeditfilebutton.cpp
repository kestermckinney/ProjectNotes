// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pnlineeditfilebutton.h"

#include <QPushButton>
#include <QWidget>
#include <QStyle>
#include <QResizeEvent>
#include <QHBoxLayout>
#include <QFileDialog>

PNLineEditFileButton::PNLineEditFileButton(QWidget *t_parent ) : QWidget( t_parent )
{
    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_line_edit = new QLineEdit( this );
    layout->addWidget( m_line_edit );

    connect( m_line_edit, SIGNAL( textChanged( const QString & ) ),
             this, SIGNAL( fileNameChanged( const QString & ) ) );

    m_push_button = new QPushButton( "...", this );
    m_push_button->setFixedWidth( this->height() );

    layout->addWidget( m_push_button );

    connect( m_push_button, SIGNAL( clicked() ),
             this, SLOT( chooseFile() ) );

    setFocusProxy( m_line_edit );
}

PNLineEditFileButton::~PNLineEditFileButton()
{

}

void PNLineEditFileButton::setFileName( const QString &fn )
{
    m_line_edit->setText( fn );
}

QString PNLineEditFileButton::fileName() const
{
    return m_line_edit->text();
}

void PNLineEditFileButton::chooseFile()
{
    QString fn;

    fn = QFileDialog::getOpenFileName( this );

    if ( !fn.isEmpty() ) {
        m_line_edit->setText( fn );
        emit fileNameChanged( fn );
    }
}

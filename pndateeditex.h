/**************************************************************************
**
** Copyright (c) 2013 Qualiant Software GmbH
**
** Author: Andreas Holzammer, KDAB (andreas.holzammer@kdab.com)
**
** Contact: Qualiant Software (d.oberkofler@qualiant.at)
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Qualiant Software at d.oberkofler@qualiant.at.
**
**************************************************************************/

#ifndef DATEEDITEX_H
#define DATEEDITEX_H

#include "widgets_export.h"
#include <QDateEdit>
#include <QLineEdit>
#include <QWheelEvent>

class WIDGETS_EXPORT PNDateEditEx : public QDateEdit
{
    Q_OBJECT

    Q_PROPERTY(bool nullable READ isNullable WRITE setNullable)
    Q_PROPERTY(bool null READ isNull)
public:
    explicit PNDateEditEx(QWidget *parent = nullptr);
    ~PNDateEditEx();

    QDateTime dateTime() const;
    QDate date() const;
    QTime time() const;

    bool isNullable() const;
    bool isNull() const;
    void setNullable(bool enable);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    QLineEdit* getLineEdit() const;

protected:
    /*! \reimp */ void showEvent(QShowEvent *event);
    /*! \reimp */ void resizeEvent(QResizeEvent *event);
    /*! \reimp */ void paintEvent(QPaintEvent *event);
    /*! \reimp */ void keyPressEvent(QKeyEvent *event);
    /*! \reimp */ void mousePressEvent(QMouseEvent *event);
    /*! \reimp */ bool focusNextPrevChild(bool next);
    /*! \reimp */ QValidator::State validate(QString &input, int &pos) const;
    /*! \reimp */ void wheelEvent(QWheelEvent *t_event) override;

public Q_SLOTS:
    /*! \reimp */ void setDateTime(const QDateTime &dateTime);
    /*! \reimp */ void setDate(const QDate &date);
    /*! \reimp */ void setTime(const QTime &time);

private:
    Q_DISABLE_COPY(PNDateEditEx)
    class Private;
    friend class Private;
    Private* d;

};

#endif // DATEEDITEX_H

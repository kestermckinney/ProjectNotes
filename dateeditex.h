// Copyright (C) 2026 Paul McKinney
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
#include <memory>

class WIDGETS_EXPORT DateEditEx : public QDateEdit
{
    Q_OBJECT

    Q_PROPERTY(bool nullable READ isNullable WRITE setNullable)
    Q_PROPERTY(bool null READ isNull)
public:
    explicit DateEditEx(QWidget *parent = nullptr);
    ~DateEditEx();

    QDateTime dateTime() const;
    QDate date() const;
    QTime time() const;

    bool isNullable() const;
    bool isNull() const;
    void setNullable(bool enable);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QLineEdit* getLineEdit() const;

protected:
    /*! \reimp */ void showEvent(QShowEvent *event) override;
    /*! \reimp */ void resizeEvent(QResizeEvent *event) override;
    /*! \reimp */ void paintEvent(QPaintEvent *event) override;
    /*! \reimp */ void keyPressEvent(QKeyEvent *event) override;
    /*! \reimp */ void mousePressEvent(QMouseEvent *event) override;
    /*! \reimp */ bool focusNextPrevChild(bool next) override;
    /*! \reimp */ QValidator::State validate(QString &input, int &pos) const override;
    /*! \reimp */ void wheelEvent(QWheelEvent *event) override;

public Q_SLOTS:
    /*! \reimp */ void setDateTime(const QDateTime &dateTime);
    /*! \reimp */ void setDate(const QDate &date);
    /*! \reimp */ void setTime(const QTime &time);

private:
    Q_DISABLE_COPY(DateEditEx)
    class Private;
    friend class Private;
    std::unique_ptr<Private> d;

};

#endif // DATEEDITEX_H

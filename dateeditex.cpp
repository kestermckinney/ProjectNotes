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

#include "dateeditex.h"
#include <QStyle>
#include <QPushButton>
#include <QLineEdit>
#include <QStyleOptionSpinBox>
#include <QKeyEvent>
#include <QCalendarWidget>
#include <QBoxLayout>
#include <QDate>

/*!
  \class DateEditEx dateeditex.h
  \brief A DateEdit with a nullable date

  This is a subclass of QDateEdit that has the additional feature
  of allowing to select a empty date. This can be achived with the
  clear button or by selecting the whole date and press backspace.

  To set an empty date from code, use the setter with an invalid
  date. To check weather the date is empty check if the date is valid,
  which comes from the getter.

  \sa QDateEdit
*/

class DateEditEx::Private {
public:
    Private( DateEditEx* qq ) : q(qq),  /*clearButton(0),*/ null(false), nullable(true), cachedLineEdit(nullptr) {}

    DateEditEx* const q;

    bool null;
    bool nullable;
    QLineEdit* cachedLineEdit;

    QLineEdit* lineEdit()
    {
        if (!cachedLineEdit)
            cachedLineEdit = q->findChild<QLineEdit *>("qt_spinbox_lineedit");
        return cachedLineEdit;
    }

    void setNull(bool n)
    {
        null = n;
        if (null)
        {
            QLineEdit *edit = lineEdit();
            if (!edit->text().isEmpty())
            {
                edit->clear();
            }
        }
    }

    QLineEdit* getLineEdit()
    {
        return lineEdit();
    }
};

/*!
  \reimp
*/
DateEditEx::DateEditEx(QWidget *parent) :
    QDateEdit(parent), d(new Private(this))
{
    setCalendarPopup(true);

    QCalendarWidget* cal = calendarWidget();

    QPushButton* todayButton = new QPushButton(tr("Today"), cal);

    QBoxLayout* calLayout = qobject_cast<QBoxLayout*>(cal->layout());
    if (calLayout)
        calLayout->addWidget(todayButton);

    connect(todayButton, &QPushButton::clicked, this, [this]() {
        setDate(QDate::currentDate());
        calendarWidget()->parentWidget()->hide();
    });
}

DateEditEx::~DateEditEx()
{
}

QSize DateEditEx::sizeHint() const
{
    return QDateEdit::sizeHint();
}

QSize DateEditEx::minimumSizeHint() const
{
    return QDateEdit::minimumSizeHint();
}

/*!
 * \brief returns date, if empty date is invalid
 * \return date, if empty date is invalid
 */
QDateTime DateEditEx::dateTime() const
{
    if (d->nullable && d->null) {
        return QDateTime();
    } else {
        return QDateEdit::dateTime();
    }
}

/*!
 * \brief returns date, if empty date is invalid
 * \return date, if empty date is invalid
 */
QDate DateEditEx::date() const
{
    if (d->nullable && d->null) {
        return QDate();
    } else {
        return QDateEdit::date();
    }
}

/*!
 * \brief returns date, if empty date is invalid
 * \return date, if empty date is invalid
 */
QTime DateEditEx::time() const
{
    if (d->nullable && d->null) {
        return QTime();
    } else {
        return QDateEdit::time();
    }
}

/*!
 * \brief sets a date, if date is invalid a
 * empty date is shown
 */
void DateEditEx::setDateTime(const QDateTime &dateTime)
{
    if (d->nullable && !dateTime.isValid()) {
        d->setNull(true);
    } else {
        d->setNull(false);
        QDateEdit::setDateTime(dateTime);
    }
}

/*!
 * \brief sets a date, if date is invalid a
 * empty date is shown
 */
void DateEditEx::setDate(const QDate &date)
{
    if (d->nullable && !date.isValid()) {
        d->setNull(true);
    } else {
        d->setNull(false);
        QDateEdit::setDate(date);
    }
}

/*!
 * \brief sets a date, if date is invalid a
 * empty date is shown
 */
void DateEditEx::setTime(const QTime &time)
{
    if (d->nullable && !time.isValid()) {
        d->setNull(true);
    } else {
        d->setNull(false);
        QDateEdit::setTime(time);
    }
}

/*!
 * \brief returns date can be empty
 * \return true, if date can be emtpy
 */
bool DateEditEx::isNullable() const
{
    return d->nullable;
}

bool DateEditEx::isNull() const
{
    return d->null;
}

/*!
 * \brief sets weahter the date can be empty
 */
void DateEditEx::setNullable(bool enable)
{
    d->nullable = enable;

    update();
}

void DateEditEx::showEvent(QShowEvent *event)
{
    QDateEdit::showEvent(event);
    d->setNull(d->null); // force empty string back in
}

/*!
  \reimp
*/
void DateEditEx::resizeEvent(QResizeEvent *event)
{
    QDateEdit::resizeEvent(event);
}

/*!
  \reimp
*/
void DateEditEx::paintEvent(QPaintEvent *event)
{
    QDateEdit::paintEvent(event);
    d->setNull(d->null); // force empty string back in
}

/*!
  \reimp
*/
void DateEditEx::keyPressEvent(QKeyEvent *event)
{
    if (d->nullable &&
        (event->key() >= Qt::Key_0) &&
        (event->key() <= Qt::Key_9) &&
        d->null) {
            setDateTime(QDateTime::currentDateTime());
        }
    if (event->key() == Qt::Key_Tab && d->nullable && d->null) {
        QAbstractSpinBox::keyPressEvent(event);
        return;
    }
    if ( ( event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete )
            && d->nullable){
        QLineEdit *edit = findChild<QLineEdit *>("qt_spinbox_lineedit");
        if (edit->selectedText() == edit->text()) {
            setDateTime(QDateTime());
            event->accept();
            return;
        }
    }

    QDateEdit::keyPressEvent(event);
}

/*!
  \reimp
*/
void DateEditEx::mousePressEvent(QMouseEvent *event)
{
    bool saveNull = d->null;
    QDateEdit::mousePressEvent(event);
    if (d->nullable && saveNull && calendarWidget()->isVisible()) {
        setDateTime(QDateTime::currentDateTime());
    }
}

void DateEditEx::wheelEvent(QWheelEvent *event)
{
    event->ignore();  // this helps to avoid accidental value chages
}

/*!
  \reimp
*/
bool DateEditEx::focusNextPrevChild(bool next)
{
    if (d->nullable && d->null){
        return QAbstractSpinBox::focusNextPrevChild(next);
    } else {
        return QDateEdit::focusNextPrevChild(next);
    }
}

QLineEdit* DateEditEx::getLineEdit() const
{
    return d->getLineEdit();
}

QValidator::State DateEditEx::validate(QString &input, int &pos) const
{
    if (d->nullable && d->null){
        return QValidator::Acceptable;
    }
    return QDateEdit::validate(input, pos);
}

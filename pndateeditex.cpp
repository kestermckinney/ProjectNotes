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

#include "pndateeditex.h"
#include <QStyle>
#include <QPushButton>
#include <QLineEdit>
#include <QStyleOptionSpinBox>
#include <QKeyEvent>
#include <QCalendarWidget>

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

class PNDateEditEx::Private {
public:
    Private( PNDateEditEx* qq ) : q(qq),  /*clearButton(0),*/ null(false), nullable(false) {}

    PNDateEditEx* const q;

    bool null;
    bool nullable;

    void setNull(bool n)
    {
        null = n;
        if (null)
        {
            QLineEdit *edit = q->findChild<QLineEdit *>("qt_spinbox_lineedit");
            if (!edit->text().isEmpty())
            {
                edit->clear();
            }
        }
    }

    QLineEdit* getLineEdit()
    {
        QLineEdit *edit = q->findChild<QLineEdit *>("qt_spinbox_lineedit");
        return edit;
    }
};

/*!
  \reimp
*/
PNDateEditEx::PNDateEditEx(QWidget *parent) :
    QDateEdit(parent), d(new Private(this))
{
}

/*!
 * \brief returns date, if empty date is invalid
 * \return date, if empty date is invalid
 */
QDateTime PNDateEditEx::dateTime() const
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
QDate PNDateEditEx::date() const
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
QTime PNDateEditEx::time() const
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
void PNDateEditEx::setDateTime(const QDateTime &dateTime)
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
void PNDateEditEx::setDate(const QDate &date)
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
void PNDateEditEx::setTime(const QTime &time)
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
bool PNDateEditEx::isNullable() const
{
    return d->nullable;
}

bool PNDateEditEx::isNull() const
{
    return d->null;
}

/*!
 * \brief sets weahter the date can be empty
 */
void PNDateEditEx::setNullable(bool enable)
{
    d->nullable = enable;

    update();
}

/*!
  \reimp
*/
QSize PNDateEditEx::sizeHint() const
{
    const QSize sz = QDateEdit::sizeHint();

    return sz;
}

/*!
  \reimp
*/
QSize PNDateEditEx::minimumSizeHint() const
{
    const QSize sz = QDateEdit::minimumSizeHint();

    return sz;
}

void PNDateEditEx::showEvent(QShowEvent *event)
{
    QDateEdit::showEvent(event);
    d->setNull(d->null); // force empty string back in
}

/*!
  \reimp
*/
void PNDateEditEx::resizeEvent(QResizeEvent *event)
{
    QDateEdit::resizeEvent(event);
}

/*!
  \reimp
*/
void PNDateEditEx::paintEvent(QPaintEvent *event)
{
    QDateEdit::paintEvent(event);
    d->setNull(d->null); // force empty string back in
}

/*!
  \reimp
*/
void PNDateEditEx::keyPressEvent(QKeyEvent *event)
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
    if (event->key() == Qt::Key_Backspace && d->nullable){
        //QLineEdit *edit = qFindChild<QLineEdit *>(this, "qt_spinbox_lineedit");
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
void PNDateEditEx::mousePressEvent(QMouseEvent *event)
{
    bool saveNull = d->null;
    QDateEdit::mousePressEvent(event);
    if (d->nullable && saveNull && calendarWidget()->isVisible()) {
        setDateTime(QDateTime::currentDateTime());
    }
}

/*!
  \reimp
*/
bool PNDateEditEx::focusNextPrevChild(bool next)
{
    if (d->nullable && d->null){
        return QAbstractSpinBox::focusNextPrevChild(next);
    } else {
        return QDateEdit::focusNextPrevChild(next);
    }
}

QLineEdit* PNDateEditEx::getLineEdit() const
{
    return d->getLineEdit();
}

QValidator::State PNDateEditEx::validate(QString &input, int &pos) const
{
    if (d->nullable && d->null){
        return QValidator::Acceptable;
    }
    return QDateEdit::validate(input, pos);
}

#ifndef PNLINEEDITFILEBUTTONDELEGATE_H
#define PNLINEEDITFILEBUTTONDELEGATE_H

#include <QStyledItemDelegate>

class PNLineEditFileButtonDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    PNLineEditFileButtonDelegate(QObject *t_parent);

    QWidget *createEditor(QWidget *t_parent, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void setEditorData(QWidget *t_editor, const QModelIndex &t_index) const;
    void setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const;
    void updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;

private:

};

#endif // PNLINEEDITFILEBUTTONDELEGATE_H

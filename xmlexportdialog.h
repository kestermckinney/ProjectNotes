#ifndef XMLEXPORTDIALOG_H
#define XMLEXPORTDIALOG_H

#include <QDialog>

namespace Ui {
class XMLExportDialog;
}

class XMLExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit XMLExportDialog(QWidget *parent = nullptr);
    ~XMLExportDialog();

    void setTableDisplayName(const QString& t_displayname);
    void setRecordKey(const QString& t_recordkey);

private slots:
    void on_buttonBox_accepted();

    void on_pushButtonFileSelect_clicked();

private:
    Ui::XMLExportDialog *ui;
};

#endif // XMLEXPORTDIALOG_H

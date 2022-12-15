#ifndef PROJECTNOTESPAGE_H
#define PROJECTNOTESPAGE_H

#include "pnbasepage.h"
#include <QObject>
#include <QDataWidgetMapper>
#include "projectnotesdelegate.h"

class ProjectNotesPage : public PNBasePage
{
    Q_OBJECT

public:
    ProjectNotesPage();
    ~ProjectNotesPage();

    void newRecord() override;

    void setupModels( Ui::MainWindow *t_ui ) override;

public slots:
    void toFirst();
    void toFirst( const QModelIndex& topLeft, const QModelIndex& bottomRight) { Q_UNUSED(topLeft);Q_UNUSED(bottomRight) toFirst(); }

private:
    Ui::MainWindow *ui = nullptr;

    QDataWidgetMapper* m_mapperProjectNotes = nullptr;

    ProjectNotesDelegate* m_project_notes_delegate = nullptr;

private slots:
    void on_tabWidgetNotes_currentChanged(int index);
};

#endif // PROJECTNOTESPAGE_H

#include "projectlocationsmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>
#include <QApplication>

ProjectLocationsModel::ProjectLocationsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjedtLocationsModel");

    setBaseSql("SELECT location_id, project_id, location_type, location_description, full_path FROM project_locations");

    setTableName("project_locations", "Project Locations");

    addColumn(0, tr("Location ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(2, tr("Location Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(3, tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

    //addRelatedTable("item_tracker", "assigned_to", "Assigned Item");

    setOrderBy("Description");
}

bool ProjectLocationsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();



    qr.setValue("project_id", *t_fk_value1);
/*
    // STOPPED HERE: Need to setup the Locations Dialog
    qr.setValue(1, QString("%1").arg(itemnumber_int, 4, 10, QLatin1Char('0')));  // Need to make a counter that looks good for items
    qr.setValue(2, "Tracker");
    qr.setValue(4, global_DBObjects.getProjectManager()); // default identified by to the pm
    qr.setValue(5, curdate); // default to today
    qr.setValue(8, "High"); // set a default priority
    qr.setValue(9, "New"); // set a default status
    qr.setValue(10, QVariant());
    qr.setValue(11, curdate); // date data as entered
    qr.setValue(12, QVariant());
    qr.setValue(15, 0);
*/
    return addRecord(qr);
}

#include "itemdetailteamlistmodel.h"

ItemDetailTeamListModel::ItemDetailTeamListModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setBaseSql("SELECT teammember_id, b.name, project_id, a.people_id FROM project_people a join people b on a.people_id=b.people_id ");

    setTableName("people", "Team Members");

    AddColumn(0, tr("Team Member ID"), DB_STRING, false, true, true, true);
    AddColumn(1, tr("Name"), DB_STRING, true, true, false, false);
    AddColumn(2, tr("Project ID"), DB_STRING, true, true, false, false);
    AddColumn(3, tr("People ID"), DB_STRING, true, true, false, false);

    SetOrderBy("name");
}

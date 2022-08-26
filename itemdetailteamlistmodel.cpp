#include "itemdetailteamlistmodel.h"

ItemDetailTeamListModel::ItemDetailTeamListModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setBaseSql("SELECT teammember_id, b.name, project_id, a.people_id FROM project_people a join people b on a.people_id=b.people_id ");

    setTableName("people", "Team Members");

    addColumn(0, tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Name"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn(2, tr("Project ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn(3, tr("People ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique);

    setOrderBy("name");
}

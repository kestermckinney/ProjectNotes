// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "itemdetailteamlistmodel.h"

ItemDetailTeamListModel::ItemDetailTeamListModel(PNDatabaseObjects* t_dbo): PNSqlQueryModel(t_dbo)
{
    setBaseSql("SELECT teammember_id, b.name, project_id, a.people_id FROM project_people a join people b on a.people_id=b.people_id ");

    setTableName("people", "Team Members");

    addColumn("teammember_id", tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("name", tr("Name"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn("people_id", tr("People ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique,
               "people", "people_id", "name");

    setOrderBy("name");
}

// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "itemdetailteamlistmodel.h"

ItemDetailTeamListModel::ItemDetailTeamListModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setBaseSql("SELECT id, b.name, project_id, a.people_id FROM project_people a join people b on a.people_id=b.id ");

    setTableName("people", "Team Members");

    addColumn("id", tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("name", tr("Name"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique,
              "projects", "id", "project_number");
    addColumn("people_id", tr("People ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique,
               "people", "id", "name");

    setOrderBy("name");
}

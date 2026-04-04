// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "itemdetailteamlistmodel.h"

ItemDetailTeamListModel::ItemDetailTeamListModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT project_people.id, people.name, project_people.project_id, people.people_id FROM project_people join people on project_people.people_id=people.id ");

    setTableName("people", "Team Members");

    addColumn("id", tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("name", tr("Name"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique,
              "projects", "id", "project_number");
    addColumn("people_id", tr("People ID"), DBString, DBSearchable, DBRequired, DBReadOnly, DBNotUnique,
               "people", "id", "name");

    setOrderBy("name");
}

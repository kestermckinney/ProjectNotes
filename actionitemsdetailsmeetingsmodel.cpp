// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "actionitemsdetailsmeetingsmodel.h"

ActionItemsDetailsMeetingsModel::ActionItemsDetailsMeetingsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("ActionItemsDetailsMeetingsModel");
    setOrderKey(35);

    setBaseSql("SELECT id, project_id, (strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title) as meeting, internal_item FROM project_notes");

    setTableName("project_notes", "Notes");

    addColumn("id", tr("Note ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("meeting", tr("Meeting"), DBString, DBNotSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("internal_item", tr("Internal Item"), DBBool, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    setOrderBy("note_date");
    setNoExport();
}

// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "actionitemsdetailsmeetingsmodel.h"

ActionItemsDetailsMeetingsModel::ActionItemsDetailsMeetingsModel(PNDatabaseObjects* t_dbo, bool t_gui): PNSqlQueryModel(t_dbo, t_gui)
{
    setObjectName("ActionItemsDetailsMeetingsModel");
    setOrderKey(35);

    setBaseSql("SELECT note_id, project_id, (strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title) as meeting, internal_item FROM project_notes");

    setTableName("project_notes", "Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(1, tr("Project ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(2, tr("Meeting"), DBString, DBNotSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(3, tr("Internal Item"), DBBool, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    setOrderBy("note_date");
    setNoExport();
}

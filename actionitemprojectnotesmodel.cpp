// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "actionitemprojectnotesmodel.h"

ActionItemProjectNotesModel::ActionItemProjectNotesModel(PNDatabaseObjects* t_dbo): PNSqlQueryModel(t_dbo)
{
    setObjectName("ActionItemProjectNotesModel");
    setOrderKey(40);

    setBaseSql("SELECT note_id, project_id, (strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title) as meeting, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    addColumn("note_id", tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("meeting", tr("Meeting"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn("internal_item", tr("Internal Item"), DBBool, DBNotSearchable, DBRequired, DBReadOnly, DBNotUnique);

    setOrderBy("note_date");
    setNoExport();
}

// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "actionitemprojectnotesmodel.h"

ActionItemProjectNotesModel::ActionItemProjectNotesModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ActionItemProjectNotesModel");
    setOrderKey(40);

    setBaseSql("SELECT note_id, project_id, (strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) || ' ' || note_title) as meeting, internal_item FROM project_notes");

    setTableName("project_notes", "Project Notes");

    addColumn(0, tr("Note ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(2, tr("Meeting"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBNotUnique);
    addColumn(3, tr("Internal Item"), DBBool, DBNotSearchable, DBRequired, DBReadOnly, DBNotUnique);

    setOrderBy("note_date");
}

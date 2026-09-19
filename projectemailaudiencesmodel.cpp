// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "projectemailaudiencesmodel.h"
#include "databaseobjects.h"
ProjectEmailAudiencesModel::ProjectEmailAudiencesModel(DatabaseObjects *dbo) : SqlQueryModel(dbo)
{
    setObjectName("ProjectEmailAudiencesModel");
    setBaseSql("SELECT id,project_id,workflow,audience_name,people_source,company_filter,selected_company_ids_json,selected_person_ids_json,recipient_distribution_json,is_default,default_workflows_json,settings_version FROM project_email_audiences");
    setTableName("project_email_audiences", "Project Email Audiences");
    addColumn("id", tr("Audience ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique, "projects", "id", "project_number");
    addColumn("workflow", tr("Workflow"), DBString, DBNotSearchable, DBRequired, DBEditable);
    addColumn("audience_name", tr("Audience Name"), DBString, DBSearchable, DBRequired, DBEditable);
    addColumn("people_source", tr("People Source"), DBString, DBNotSearchable, DBRequired, DBEditable);
    addColumn("company_filter", tr("Company Filter"), DBString, DBNotSearchable, DBRequired, DBEditable);
    addColumn("selected_company_ids_json", tr("Selected Companies"), DBString, DBNotSearchable, DBRequired, DBEditable);
    addColumn("selected_person_ids_json", tr("Selected People"), DBString, DBNotSearchable, DBRequired, DBEditable);
    addColumn("recipient_distribution_json", tr("Recipient Distribution"), DBString, DBNotSearchable, DBRequired, DBEditable);
    addColumn("is_default", tr("Default"), DBBool, DBNotSearchable, DBRequired, DBEditable);
    addColumn("default_workflows_json", tr("Default For Reports"), DBString, DBNotSearchable, DBNotRequired, DBEditable);
    addColumn("settings_version", tr("Settings Version"), DBInteger, DBNotSearchable, DBRequired, DBEditable);
    addUniqueKeys({"project_id", "workflow", "audience_name"}, "Name");
    setOrderBy("project_id, workflow, audience_name");
}

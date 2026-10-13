// IFS Cloud REST calls: importing projects, budgets and team members, and
// keeping tracker items in step with the tasks of each project's ISSUES
// activity. Replaces IFSCommon in includes/ifs_tools.py.
import { data, http, log, settings, storage } from "projectnotes";
import { escapeHtml } from "projectnotes/common";
import { ifsUrl, token, user } from "./sso.mjs";

const PROJECTION = "/main/ifsapplications/projection/v1/";
const ACTIVE_STATES = ["Initialized", "Started", "Approved"];

function quote(text) { return "'" + String(text).replace(/'/g, "''") + "'"; }
function xml(text) { return escapeHtml(text === null || text === undefined ? "" : String(text)); }

function query(params) {
    return Object.keys(params).filter(k => params[k] !== undefined && params[k] !== "")
        .map(k => k + "=" + encodeURIComponent(params[k])).join("&");
}

// GET/POST/PATCH/DELETE against an IFS projection. Returns the response, or
// null when there is no token.
function* call(method, path, params, body, headers) {
    const bearer = yield token();
    if (!bearer) return null;
    const q = params ? query(params) : "";
    return yield http.fetch(ifsUrl() + PROJECTION + path + (q ? "?" + q : ""), {
        method: method,
        auth: { type: "bearer", token: bearer },
        headers: Object.assign({ "Prefer": "odata.maxpagesize=500" }, headers || {}),
        body: body,
        timeoutSeconds: 120,
    });
}

function* getValues(path, params) {
    const response = yield call("GET", path, params);
    if (!response) return null;
    if (!response.ok) {
        log.warn("IFS request failed (" + response.status + "): " + path + " " + (yield response.text()).slice(0, 300));
        return null;
    }
    return (yield response.json()).value || [];
}

export function hasSettings() { return ifsUrl() !== ""; }
export function syncTrackerItems() { return String(settings.get("SyncTrackerItems", "") || "").toLowerCase() === "true"; }

// ── dates ───────────────────────────────────────────────────────────────────

// IFS "2026-09-11T14:05:00Z" or "2026-09-11" -> Date (local), or null.
function ifsDate(text) {
    const m = /^(\d{4})-(\d{2})-(\d{2})(?:T(\d{2}):(\d{2}):(\d{2}))?/.exec(String(text || ""));
    return m ? new Date(+m[1], +m[2] - 1, +m[3], +(m[4] || 0), +(m[5] || 0), +(m[6] || 0)) : null;
}
function isoDate(text) { const m = /^\d{4}-\d{2}-\d{2}/.exec(String(text || "")); return m ? m[0] : ""; }
// "2026-09-11..." -> "09/11/2026", the Project Notes XML date format.
function xmlDate(text) { const d = isoDate(text); return d ? d.slice(5, 7) + "/" + d.slice(8, 10) + "/" + d.slice(0, 4) : ""; }
function enumToText(value) {
    const t = String(value || "").replace("CfEnum_", "").toLowerCase();
    return t.replace(/(^|[\s_-])([a-z])/g, (all, sep, c) => sep + c.toUpperCase());
}
function lookupName(ref) { return ref && typeof ref === "object" ? String(ref.lookup || "").trim() : String(ref || "").trim(); }

// ── IFS tasks ───────────────────────────────────────────────────────────────

function* issuesActivitySeq(projectNumber) {
    const values = yield getValues("ActivityListHandling.svc/Activities", {
        "$filter": "(ProjectId eq " + quote(projectNumber) + " and ActivityNo eq 'ISSUES')",
        "$select": "ActivitySeq,ActivityNo,Description", "$top": "1" });
    return values && values.length ? values[0].ActivitySeq : null;
}

// Null when IFS could not be asked, so that a failed request is never taken
// to mean "IFS has no tasks".
function* issueTasks(activitySeq) {
    return yield getValues("ProjectScopeAndScheduleHandling.svc/Activities(ActivitySeq=" + activitySeq + ")/ActivityTasks", {
        "$orderby": "TaskId",
        "$select": "TaskId,Name,Info,Completed,CompletedDate,Cf_Date_Updated,Cf_Assigned_To,Cf_Identified_By,"
                 + "Cf_Datedue,Cf_Dateresolved,Cf_Priority,Cf_Status" });
}

function taskBody(activitySeq, item) {
    const body = {
        Name: item.item_name || "",
        Info: item.description || "",
        Completed: false,
        Offset: 0,
        ActivitySeq: activitySeq,
        ProjectCostElement: "LABOR-INT",
        Cf_Assigned_To: lookupName(item.assigned_to),
        Cf_Identified_By: lookupName(item.identified_by),
    };
    if (isoDate(item.date_due)) body.Cf_Datedue = isoDate(item.date_due);
    if (isoDate(item.last_update)) body.Cf_Date_Updated = isoDate(item.last_update);
    if (isoDate(item.date_resolved)) body.Cf_Dateresolved = isoDate(item.date_resolved);
    if (item.priority) body.Cf_Priority = "CfEnum_" + String(item.priority).toUpperCase();
    if (item.status) body.Cf_Status = "CfEnum_" + String(item.status).toUpperCase();
    return body;
}

function* createTask(activitySeq, taskId, item) {
    const body = Object.assign({ TaskId: taskId }, taskBody(activitySeq, item));
    const response = yield call("POST", "ProjectScopeAndScheduleHandling.svc/ActivityTasks", null, body);
    if (response && response.status === 201) return true;
    if (response) log.warn("Creating IFS task " + taskId + " failed (" + response.status + "): " + (yield response.text()).slice(0, 300));
    return false;
}

function* updateTask(activitySeq, taskId, item) {
    const response = yield call("PATCH", "ProjectScopeAndScheduleHandling.svc/ActivityTasks(TaskId=" + quote(encodeURIComponent(taskId)) + ")",
                                null, taskBody(activitySeq, item), { "If-Match": "*" });
    return !!response && response.status === 200;
}

// One IFS task as an item_tracker row in Project Notes XML.
function itemRow(projectKey, itemNumber, task, itemKey) {
    let status = enumToText(task.Cf_Status);
    if (!status) status = task.Completed ? "Closed" : "Open";
    return "<row>"
        + (itemKey ? "<column name=\"id\">" + xml(itemKey) + "</column>" : "")
        + "<column name=\"item_type\">Tracker</column>"
        + "<column name=\"project_id\">" + xml(projectKey) + "</column>"
        + "<column name=\"item_number\">" + xml(itemNumber) + "</column>"
        + "<column name=\"item_name\">" + xml(task.Name) + "</column>"
        + "<column name=\"description\">" + xml(task.Info) + "</column>"
        + "<column name=\"status\">" + xml(status) + "</column>"
        + "<column name=\"priority\">" + xml(enumToText(task.Cf_Priority)) + "</column>"
        + "<column name=\"assigned_to\" lookupvalue=\"" + xml(String(task.Cf_Assigned_To || "").trim()) + "\"></column>"
        + "<column name=\"identified_by\" lookupvalue=\"" + xml(String(task.Cf_Identified_By || "").trim()) + "\"></column>"
        + "<column name=\"date_due\">" + xml(xmlDate(task.Cf_Datedue)) + "</column>"
        + "<column name=\"last_update\">" + xml(xmlDate(task.Cf_Date_Updated)) + "</column>"
        + "<column name=\"date_resolved\">" + xml(xmlDate(task.Cf_Dateresolved)) + "</column>"
        + "</row>";
}

// Brings one project's tracker items and ISSUES tasks in step.
//  - A task whose TaskId starts with the project number came from Project
//    Notes (item number = the rest); other tasks get item number "IFS" + TaskId.
//  - When both sides have the item, the newer last update wins.
//  - Items only in Project Notes are created in IFS, except internal items
//    and "IFS..." items (their IFS task was deleted).
export function* syncProjectTasks(project, tasks, activitySeq) {
    const counts = { created: 0, updated: 0, skipped: 0 };
    const items = yield data.list("item_tracker", { where: { project_id: { id: project.$key } } });
    const byNumber = {};
    const byName = {};
    items.forEach(function (item) {
        byNumber[String(item.item_number)] = item;
        if (item.item_name) byName[item.item_name] = item;
    });
    const matched = {};
    const rows = [];

    for (const task of tasks) {
        const taskId = String(task.TaskId || "");
        const itemNumber = taskId.indexOf(project.project_number) === 0 ? taskId.slice(project.project_number.length) : "IFS" + taskId;
        const ifsUpdated = ifsDate(task.Cf_Date_Updated);
        const item = byNumber[itemNumber];
        if (item) {
            matched[itemNumber] = true;
            const pnUpdated = ifsDate(item.last_update);
            if (ifsUpdated && pnUpdated && pnUpdated.getTime() > ifsUpdated.getTime()) {
                if ((yield updateTask(activitySeq, taskId, item)) || (yield createTask(activitySeq, taskId, item))) counts.updated++;
                else counts.skipped++;
            } else if (ifsUpdated && pnUpdated && pnUpdated.getTime() === ifsUpdated.getTime()) {
                counts.skipped++;
            } else {
                rows.push(itemRow(project.$key, itemNumber, task));
                counts.updated++;
            }
        } else if (task.Name && byName[task.Name]) {
            // Same name under another number: update that row instead of
            // colliding with UNIQUE(project_id, item_name).
            const existing = byName[task.Name];
            matched[String(existing.item_number)] = true;
            rows.push(itemRow(project.$key, itemNumber, task, existing.$key));
            counts.updated++;
        } else {
            rows.push(itemRow(project.$key, itemNumber, task));
            counts.created++;
        }
    }

    for (const item of items) {
        const number = String(item.item_number);
        if (matched[number] || number.indexOf("IFS") === 0 || item.internal_item === true || item.internal_item === 1 || item.internal_item === "1")
            continue;
        if (yield createTask(activitySeq, project.project_number + number, item)) counts.created++;
        else counts.skipped++;
    }

    if (rows.length)
        yield data.importXml("<projectnotes><table name=\"item_tracker\">" + rows.join("") + "</table></projectnotes>");
    return counts;
}

// Every active project's tracker items, or nothing when IFS is unreachable.
export function* syncActivityTasks() {
    const projects = yield data.list("projects", { where: { project_status: "Active" }, orderBy: ["project_number"] });
    if (!projects.length || !(yield token())) return null;
    const total = { created: 0, updated: 0, skipped: 0 };
    for (const project of projects) {
        const seq = yield issuesActivitySeq(project.project_number);
        if (seq === null || seq === undefined) continue;
        const tasks = yield issueTasks(seq);
        if (!tasks) continue;
        const counts = yield syncProjectTasks(project, tasks, seq);
        total.created += counts.created;
        total.updated += counts.updated;
        total.skipped += counts.skipped;
    }
    return total;
}

// ── project import ──────────────────────────────────────────────────────────

function* aggregate(path) {
    const values = yield getValues(path);
    return values && values.length ? values[0] : {};
}

function earnedValue(projectId) {
    return aggregate("ProjectMonitoringHandling.svc/Projects(ProjectId=" + quote(encodeURIComponent(projectId))
        + ")/ProjectAnalysisArray?$apply=" + encodeURIComponent("aggregate(Bcws with sum as Bcws_aggr_,Bcwp with sum as Bcwp_aggr_,"
        + "Acwp with sum as Acwp_aggr_,Bac with sum as Bac_aggr_,Etc with sum as Etc_aggr_,Eac with sum as Eac_aggr_,"
        + "Vac with sum as Vac_aggr_,Cv with sum as Cv_aggr_,Sv with sum as Sv_aggr_)"));
}

function costs(projectId) {
    return aggregate("ProjectMonitoringHandling.svc/Projects(ProjectId=" + quote(encodeURIComponent(projectId))
        + ")/ProjectCostArray?$apply=" + encodeURIComponent("aggregate(Estimated with sum as Estimated_aggr_,Planned with sum as Planned_aggr_,"
        + "Baseline with sum as Baseline_aggr_,EarnedValue with sum as EarnedValue_aggr_,ScheduledWork with sum as ScheduledWork_aggr_,"
        + "PlannedCommitted with sum as PlannedCommitted_aggr_,Committed with sum as Committed_aggr_,Used with sum as Used_aggr_,"
        + "Actual with sum as Actual_aggr_)"));
}

function number(value) { return value === null || value === undefined ? "" : String(value); }

// Imports the user's active IFS projects (eight per call, or all of them),
// their budgets and team members, and closes Project Notes projects that are
// no longer active in IFS. Returns the number of projects read, or null.
export function* importProjects(all) {
    if (!(yield token())) return null;
    const manager = user();
    if (!manager) {
        log.warn("IFS sign-in worked but the IFS user could not be identified; projects were not imported.");
        return null;
    }

    const groups = {};
    for (const g of (yield getValues("ResourceGroupsHandling.svc/ResourceSet", { "$select": "ResourceId,Description" })) || [])
        groups[g.ResourceId] = g.Description;

    const state = (yield storage.readJson("state.json")) || {};
    const top = all ? 300 : 8;
    const skip = all ? 0 : ((state.ifs_projects_import && state.ifs_projects_import.skip) || 0);
    const projects = yield getValues("ProjectsHandling.svc/Projects", {
        "$skip": skip > 0 ? String(skip) : undefined,
        "$top": String(top),
        "$orderby": "ProjectId",
        "$filter": "(Manager eq " + quote(manager) + " and (" + ACTIVE_STATES.map(s => "Objstate eq IfsApp.ProjectsHandling.ProjectState'" + s + "'").join(" or ") + "))",
        "$select": "ProjectId,Objstate,Name,Description,CompanyName,Manager,Cf_Lastinvoiced",
        "$expand": "CustomerIdRef($select=Name)",
    });
    if (!projects) return null;

    const companies = {};
    const people = {};
    const team = {};
    let projectRows = "", peopleRows = "", teamRows = "";

    for (const p of projects) {
        const companyName = p.CompanyName || "";
        const customer = p.CustomerIdRef && p.CustomerIdRef.Name ? p.CustomerIdRef.Name : "";
        if (customer) companies[customer] = true;
        const ev = yield earnedValue(p.ProjectId);
        const cost = yield costs(p.ProjectId);
        projectRows += "<row>"
            + "<column name=\"project_number\">" + xml(p.ProjectId) + "</column>"
            + "<column name=\"project_name\">" + xml(p.Description) + "</column>"
            + "<column name=\"last_invoice_date\">" + xml(p.Cf_Lastinvoiced) + "</column>"
            + (customer ? "<column name=\"client_id\" lookupvalue=\"" + xml(customer) + "\"></column>" : "")
            + "<column name=\"project_status\">" + (ACTIVE_STATES.indexOf(p.Objstate) >= 0 ? "Active" : "Closed") + "</column>"
            + "<column name=\"budget\">" + xml(number(cost.Baseline_aggr_)) + "</column>"
            + "<column name=\"actual\">" + xml(number(cost.Used_aggr_)) + "</column>"
            + "<column name=\"bcwp\">" + xml(number(cost.EarnedValue_aggr_)) + "</column>"
            + "<column name=\"bcws\">" + xml(number(cost.ScheduledWork_aggr_)) + "</column>"
            + "<column name=\"bac\">" + xml(number(ev.Bac_aggr_)) + "</column>"
            + "</row>";

        const allocations = (yield getValues("ProjectResourceAllocationsHandling.svc/ProjResourceAllocations", {
            "$filter": "(ProjectId eq " + quote(p.ProjectId) + ")", "$expand": "EmployeeIdRef($select=EmployeeName)" })) || [];
        for (const a of allocations) {
            if (!a.EmployeeIdRef) continue;   // older projects and templates may have no employee
            const name = a.EmployeeIdRef.EmployeeName;
            const teamKey = p.ProjectId + "\u0001" + name;
            if (!team[teamKey]) {
                team[teamKey] = true;
                teamRows += "<row><column name=\"project_id\" lookupvalue=\"" + xml(p.ProjectId) + "\"></column>"
                    + "<column name=\"people_id\" lookupvalue=\"" + xml(name) + "\"></column>"
                    + "<column name=\"role\">" + xml(groups[a.ResourceParentId] || "") + "</column></row>";
            }
            if (!people[name]) {
                people[name] = true;
                peopleRows += "<row><column name=\"name\">" + xml(name) + "</column>"
                    + (companyName ? "<column name=\"client_id\" lookupvalue=\"" + xml(companyName) + "\"></column>" : "") + "</row>";
            }
            if (companyName) companies[companyName] = true;
        }
    }

    projectRows += yield closedProjectRows();

    state.ifs_projects_import = { skip: projects.length < top ? 0 : skip + top };
    yield storage.writeJson("state.json", state);

    const companyRows = Object.keys(companies).filter(Boolean)
        .map(c => "<row><column name=\"client_name\">" + xml(c) + "</column></row>").join("");
    yield data.importXml("<projectnotes>"
        + "<table name=\"clients\">" + companyRows + "</table>"
        + "<table name=\"people\">" + peopleRows + "</table>"
        + "<table name=\"projects\">" + projectRows + "</table>"
        + "<table name=\"project_people\">" + teamRows + "</table>"
        + "</projectnotes>");
    return projects.length;
}

// Rows that close active Project Notes projects IFS no longer lists as active.
function* closedProjectRows() {
    let rows = "";
    const active = yield data.list("projects", { where: { project_status: "Active" }, fields: ["project_number"] });
    for (const project of active) {
        const values = yield getValues("ProjectsHandling.svc/Projects", {
            "$filter": "(ProjectId eq " + quote(project.project_number) + ")", "$select": "ProjectId,Objstate", "$top": "1" });
        if (!values || !values.length) continue;   // not visible to this user; leave it alone
        if (ACTIVE_STATES.indexOf(values[0].Objstate) < 0) {
            log.info(project.project_number + " is " + values[0].Objstate + " in IFS; closing it in Project Notes.");
            rows += "<row><column name=\"project_number\">" + xml(project.project_number) + "</column>"
                  + "<column name=\"project_status\">Closed</column></row>";
        }
    }
    return rows;
}

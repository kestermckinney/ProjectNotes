from includes.common import ProjectNotesCommon
from PyQt6 import QtCore
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo, QElapsedTimer
from urllib3.exceptions import InsecureRequestWarning 

import requests
import inspect
import json
from urllib3.exceptions import InsecureRequestWarning 

import projectnotes

requests.packages.urllib3.disable_warnings(category=InsecureRequestWarning)

class IFSCommon:
    def __init__(self):
        self.settings_pluginname = "IFS Cloud"

        self.pnc = ProjectNotesCommon()

        self.ifs_username = self.pnc.get_plugin_setting("UserName", self.settings_pluginname)
        self.ifs_url = self.pnc.get_plugin_setting("URL", self.settings_pluginname)
        self.ifs_password = self.pnc.get_plugin_setting("Password", self.settings_pluginname)
        self.ifs_person_id = self.pnc.get_plugin_setting("PersonId", self.settings_pluginname)
        self.report_server = self.pnc.get_plugin_setting("ReportServer", self.settings_pluginname)
        self.domain_user = self.pnc.get_plugin_setting("DomainUser", self.settings_pluginname)
        self.domain_password = self.pnc.get_plugin_setting("DomainPassword", self.settings_pluginname)
        self.sync_tracker_items = self.pnc.get_plugin_setting("SyncTrackerItems", self.settings_pluginname)


    def get_has_settings(self):
        if self.ifs_username is None or self.ifs_username == '':
            return False

        if self.ifs_url is None or self.ifs_url == '':
            return False

        if self.ifs_password is None or self.ifs_password == '':
            return False

        if self.ifs_person_id is None or self.ifs_person_id == '':
            return False

        if self.report_server is None or self.report_server == '':
            return False

        if self.domain_user is None or self.domain_user == '':
            return False

        if self.domain_password is None or self.domain_password == '':
            return False

        return True

    def get_sync_tracker_items(self):
        return self.sync_tracker_items

    def get_earned_value_metrics(self, projectid):
            request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectMonitoringHandling.svc/Projects(ProjectId=%27' + projectid + '%27)/ProjectAnalysisArray?$apply=aggregate(Bcws%20with%20sum%20as%20Bcws_aggr_,Bcwp%20with%20sum%20as%20Bcwp_aggr_,Acwp%20with%20sum%20as%20Acwp_aggr_,Bac%20with%20sum%20as%20Bac_aggr_,Etc%20with%20sum%20as%20Etc_aggr_,Eac%20with%20sum%20as%20Eac_aggr_,Vac%20with%20sum%20as%20Vac_aggr_,Cv%20with%20sum%20as%20Cv_aggr_,Sv%20with%20sum%20as%20Sv_aggr_)'

            result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

            if (result.status_code != 200):
                print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
                return ""

            json_result = result.json()

            return(json_result['value'][0])

    def get_cost_metrics(self, projectid):
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectMonitoringHandling.svc/Projects(ProjectId=%27' + projectid + '%27)/ProjectCostArray?$apply=aggregate(Estimated%20with%20sum%20as%20Estimated_aggr_,Planned%20with%20sum%20as%20Planned_aggr_,Baseline%20with%20sum%20as%20Baseline_aggr_,EarnedValue%20with%20sum%20as%20EarnedValue_aggr_,ScheduledWork%20with%20sum%20as%20ScheduledWork_aggr_,PlannedCommitted%20with%20sum%20as%20PlannedCommitted_aggr_,Committed%20with%20sum%20as%20Committed_aggr_,Used%20with%20sum%20as%20Used_aggr_,Actual%20with%20sum%20as%20Actual_aggr_)'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        return(json_result['value'][0])

    def get_open_activities(self, projectid):
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ActivityListHandling.svc/Activities?$filter=((((Objstate%20eq%20IfsApp.ActivityListHandling.ActivityState%27Planned%27%20or%20Objstate%20eq%20IfsApp.ActivityListHandling.ActivityState%27Released%27))%20and%20(ProjectId%20eq%20%27' + projectid + '%27)))&$orderby=ShortName,ActivityNo&$select=ProjectId,Description,EarlyStartDate,EarlyFinishDate,Manager,Company,CCusPoSeq,ActivitySeq,Objstate,Objgrants,ProgressCost,ProgressHours,ActivityNo,ProgressTemplate,SubProjectId,AccessOn,EarlyStart,ActualStart,EarlyFinish,ActualFinish,TotalWorkDays,ShortName,ProgressMethod,ExcludeResourceProgress,ManualProgressLevel,ManualProgressCost,ManualProgressHours,EstimatedProgress,ProgressTemplateStep,PlannedCostDriver,Note,LateStart,LateFinish,luname,keyref&$expand=ProjectRef($select=Name,Objgrants,luname,keyref),SubProjectRef($select=Description,Objgrants,luname,keyref)'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        return(json_result)

    def get_activity_tasks(self, activityseq):
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectScopeAndScheduleHandling.svc/Activities(ActivitySeq=' + str(activityseq) + ')/ActivityTasks?$orderby=TaskId&$select=TaskId,Name,Info,Completed,Objgrants,CompletedDate,luname,keyref'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        return(json_result)
        
    def get_status_items(self, project_num, json_data, dayspan):
        xml = '<table name="status_report_items">\n'

        if 'value' in json_data:
            for j in json_data['value']:
                if 'luname' in j:
                    if j['luname'] == 'Activity' and j['ProgressMethod'] == 'Manual' and j['EarlyStart'] is not None and j['EarlyFinish'] is not None: 

                        earlystart = QDateTime.fromString(j['EarlyStart'][0:10], 'yyyy-MM-dd')
                        earlyfinish = QDateTime.fromString(j['EarlyFinish'][0:10], 'yyyy-MM-dd')
                        
                        finishrange = QDateTime.currentDateTime().daysTo(earlyfinish)
                        startrange = QDateTime.currentDateTime().daysTo(earlystart)

                        status = j['Objstate']

                        showitem = False
                        inprogress = False
                        willstart = False
                        hascompleted = False

                        if (startrange < 0 and status != 'Completed' and status != 'Closed'):
                            # show if started and not complete
                            inprogress = True
                            showitem = True

                        if (finishrange >= -dayspan and (status == 'Completed' or status == 'Closed')):
                            # show item if closed in the past day range
                            hascompleted = True
                            showitem = True

                        if (startrange <= dayspan and startrange > 0 and status != 'Completed' and status != 'Closed'):
                            # show item if starting in the next day range
                            willstart = True
                            showitem = True

                        if showitem:
                            xml = xml + '<row earlystart="' + j['EarlyStart'] +'" EarlyFinish="' + j['EarlyFinish'] + '" startrange="' + str(startrange) + '" finishrange="' + str(finishrange) + '" objstate="' + status + '">\n'

                            xml = xml + '<column name="project_id" lookupvalue="' + project_num + '"></column>\n'

                            progress = str(round(float(0 if j['EstimatedProgress'] is None else j['EstimatedProgress']) * 100.0, 0)) + '%'

                            if hascompleted:
                                xml = xml + '<column name="task_category">Completed</column>\n'
                            elif willstart:
                                xml = xml + '<column name="task_category">Starting</column>\n'
                            else: 
                                xml = xml + '<column name="task_category">In Progress</column>\n'

                            xml = xml + '<column name="task_description">' + j['Description'] + ' (' + progress + ')'

                            if (j['Note'] is not None):
                                xml = xml + ' - ' + str(j['Note'])

                            xml = xml +  '</column>\n'

                            xml = xml + '</row>\n'

        xml = xml + '</table>\n'

        return(xml)

    def create_activity_task(self, activityseq, projectid, taskid, name, description, assignedto, dateupdated, datedue, dateresolved, identifiedby, priority, status):
        docdata = {
            "TaskId": taskid,
            "Name": name,
            "Info": description,
            "Completed": False,
            "Offset": 0,
            "ActivitySeq": activityseq,
            "ProjectCostElement": "LABOR-INT",
            "Cf_Assigned_To" : assignedto,
            "Cf_Identified_By" : identifiedby,
            }

        if datedue.isValid():
            docdata["Cf_Datedue"] = datedue.toString("yyyy-MM-dd")

        if dateupdated.isValid():
            docdata["Cf_Date_Updated"] = dateupdated.toString("yyyy-MM-dd")

        if dateresolved.isValid():
            docdata["Cf_Dateresolved"] = dateresolved.toString("yyyy-MM-dd")

        if not priority is None:
            docdata["Cf_Priority"] = "CfEnum_" + priority.upper()

        if not status is None:
            docdata["Cf_Status"] = "CfEnum_" + status.upper()

        # print("sending doc:")
        # print(json.dumps(docdata, indent=4))

        request_url = self.ifs_url + "/main/ifsapplications/projection/v1/create_activity_task.svc/TaskSet"

        print(f"Creating Activity in IFS, makeing url request: {request_url}")

        result = requests.post(request_url, verify=False, auth=(self.ifs_username, self.ifs_password), json=docdata, headers={"Content-Type": "application/json"})
        
        if (result.status_code != 201):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")

            # print("Debug JSON: create_activity_task")
            # json_result = result.json()
            # print(json.dumps(json_result, indent=4))

            return False

        return True

    def update_activity_task(self, activityseq, projectid, taskid, name, description, assignedto, dateupdated, datedue, dateresolved, identifiedby, priority, status):
        docdata = {
            "Name": name,
            "Info": description,
            "Completed": False,
            "Offset": 0,
            "ActivitySeq": activityseq,
            "ProjectCostElement": "LABOR-INT",
            "Cf_Assigned_To" : assignedto,
            "Cf_Identified_By" : identifiedby,
            }

        if datedue.isValid():
            docdata["Cf_Datedue"] = datedue.toString("yyyy-MM-dd")

        if dateupdated.isValid():
            docdata["Cf_Date_Updated"] = dateupdated.toString("yyyy-MM-dd")

        if dateresolved.isValid():
            docdata["Cf_Dateresolved"] = dateresolved.toString("yyyy-MM-dd")

        if not priority is None:
            docdata["Cf_Priority"] = "CfEnum_" + priority.upper()
            
        if not status is None:
            docdata["Cf_Status"] = "CfEnum_" + status.upper()

        request_url = self.ifs_url + "/int/ifsapplications/entity/v1/ActivityTaskEntity.svc/ActivityTaskSet(TaskId='" + taskid + "')"

        #print(f"Updating Activity in IFS, makeing url request: {request_url}")

        result = requests.patch(request_url, verify=False, auth=(self.ifs_username, self.ifs_password), json=docdata, headers={"Content-Type": "application/json"})
        
        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return False

        return True

    def delete_activity_task(self, activityseq, projectid, taskid):
        docdata = {
            "Name": "Works like a champ",
            "Info": "It is a Text",
            "HoursPlanned": 1,
            "Completed": False,
            "Offset": 1,
            "ActivitySeq": activityseq,
            "ProjectCostElement": "LABOR-INT",
            }

        request_url = self.ifs_url + "/int/ifsapplications/entity/v1/ActivityTaskEntity.svc/ActivityTaskSet(TaskId='" + taskid + "')"

        #print(f"Deleting Activity in IFS, makeing url request: {request_url}")

        result = requests.delete(request_url, verify=False, auth=(self.ifs_username, self.ifs_password), json=docdata, headers={"Content-Type": "application/json", "If-Match": "*"})
        
        if (result.status_code != 204):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return False

        return True

    def get_issues_activity(self, json_data):
        if 'value' in json_data:
            for j in json_data['value']:
                if 'ActivitySeq' in j:
                    if j['ActivityNo'] == 'ISSUES':      
                        return j['ActivitySeq']

        return None

    def download_report(self, reporturl, savelocation):
        result = requests.get(reporturl,  auth=(self.domain_user, self.domain_password))

        if (result.status_code != 200):
            print(f"File Download Failed {result.reason}: {result.text}")
            return False

        QFile.remove(savelocation) 
        with open(savelocation, mode="wb") as file:
            file.write(result.content)

        return True

    def getprojectsxml(self, rgroups, clientsdict, rd, parameter):

        saved_state = None
        statename = "ifs_projects_import"
        skip = 0
        top = 10

        projectcount = 0

        segment = ""

        if parameter == "all":
            top = 300
        else:
            skip = self.pnc.get_save_state(statename)

        if (skip > 0):
            segment = segment + f"$skip={skip}"

        if (skip > 0 and top > 0):
            segment = segment + "&"

        if (top > 0):
            segment = segment + f"$top={top}"

        if (skip > 0 or top > 0):
         segment = segment + "&"

        segment = segment + "$orderby=ProjectId&"

        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectsHandling.svc/Projects?' + segment + '$filter=(((Objstate%20eq%20IfsApp.ProjectsHandling.ProjectState%27Initialized%27%20or%20Objstate%20eq%20IfsApp.ProjectsHandling.ProjectState%27Started%27%20or%20Objstate%20eq%20IfsApp.ProjectsHandling.ProjectState%27Approved%27))%20and%20Manager%20eq%20%27' + self.ifs_person_id + '%27)&$select=BudgetControlOn,ControlAsBudgeted,ControlOnTotalBudget,ProjUniquePurchase,ProjUniqueSale,State,ProjectId,Objstate,Objgrants,Name,Company,CustomerCategory,CustomerId,FinancialProjectExist,History,DefaultSite,ProjectPngExists,CheckForecast,Description,CompanyName,Manager,AccessOnOff,PlanStart,PlanFinish,ActualStart,ActualFinish,ApprovedDate,CloseDate,CancelDate,FrozenDate,EarnedValueMethod,BaselineRevisionNumber,Cf_Lastinvoiced,luname,keyref&$expand=AccountingProjectRef($select=ProjectGroup,Objgrants,luname,keyref),ManagerRef($select=Name,luname,keyref),CustomerIdRef($select=Name,Objgrants,luname,keyref)'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        #print(f"Query for projects is : {request_url}")

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        #print("Debug JSON:")
        #print(json.dumps(json_result, indent=4))

        for rowval in json_result['value']:

            projectcount = projectcount + 1

            rd['companyname'] = rowval['CompanyName']

            rd['projectsxmlrows'] +=  "  <row>\n"

            rd['projectsxmlrows'] +=  "    <column name=\"project_number\">" + self.pnc.to_xml(rowval['ProjectId']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"project_name\">" + self.pnc.to_xml(rowval['Description']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"last_invoice_date\">" + self.pnc.to_xml(rowval['Cf_Lastinvoiced']) + "</column>\n"

            if rowval['CustomerIdRef'] is not None:
                rd['projectsxmlrows'] +=  "    <column name=\"client_id\" lookupvalue=\"" + self.pnc.to_xml(rowval['CustomerIdRef']['Name']) + "\"></column>\n"
                
                # only add the company name once to the client list
                clientsdict[rowval['CustomerIdRef']['Name']] = True

            rd['projectsxmlrows'] +=  "    <column name=\"project_status\">Active</column>\n"

            metrics = self.get_earned_value_metrics(rowval['ProjectId'])
            costmetrics = self.get_cost_metrics(rowval['ProjectId'])

            rd['projectsxmlrows'] +=  "    <column name=\"budget\">" + str(costmetrics['Baseline_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"actual\">" + str(costmetrics['Used_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"bcwp\">" + str(costmetrics['EarnedValue_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"bcws\">" + str(costmetrics['ScheduledWork_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"bac\">" + str(metrics['Bac_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "  </row>\n"

            #TODO: maybe just let file finder do this on  it's own rd['projectlocationsxmlrows'] = rd['projectlocationsxmlrows'] + self.pnc.find_projectlocations( rowval['ProjectId'], ProjectsFolder)

            self.get_team_members_xml(rgroups, clientsdict, rowval['ProjectId'], rd )

        self.pnc.set_save_state(statename, skip, top, projectcount)

            
    def get_resource_groups(self, rgroups):
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ResourceGroupsHandling.svc/ResourceSet?$select=ResourceId,Description'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Content-Type" : "application/json"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        #print(result.content)

        json_result = result.json()

        #print("Debug JSON:")
        #print(json.dumps(json_result, indent=4))

        for value in json_result['value']:
            rgroups[value['ResourceId']] = value['Description']        


    def get_team_members_xml(self, rgroups, clientsdict, projectid, rd):
        #TODO: This broke in 25R1 need to fix
        return
        
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectResourcePlanningHandling.svc/ProjectSet(ProjectId=%27' + projectid + '%27)/ProjectAllocationArray?$apply=groupby((EmployeeName,ResourceId))'
        
        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        #print("Debug JSON:")
        #print(json.dumps(json_result, indent=4))

        for rowval in json_result['value']:

            rd['projectpeoplexmlrows'] += "  <row>\n"

            rd['projectpeoplexmlrows'] += "    <column name=\"project_id\" lookupvalue=\"" + self.pnc.to_xml(projectid) + "\"></column>\n"
            rd['projectpeoplexmlrows'] += "    <column name=\"people_id\" lookupvalue=\"" + self.pnc.to_xml(rowval['EmployeeName']) + "\"></column>\n"
            rd['projectpeoplexmlrows'] += "    <column name=\"role\">" + self.pnc.to_xml(rgroups[rowval['ResourceId']]) + "</column>\n"
            rd['projectpeoplexmlrows'] += "  </row>\n"

            rd['peoplexmlrows'] += "  <row>\n"

            rd['peoplexmlrows'] += "    <column name=\"name\">" + self.pnc.to_xml(rowval['EmployeeName']) + "</column>\n"
            rd['peoplexmlrows'] += "    <column name=\"client_id\" lookupvalue=\"" + self.pnc.to_xml(rd['companyname']) + "\"></column>\n"

            rd['peoplexmlrows'] += "  </row>\n"

            # only add the company name once to the client list
            clientsdict[rd['companyname']] = True

    def import_ifs_projects(self, parameter):
        timer = QElapsedTimer()
        timer.start()

        clientsdict = {}
        rgroups = {}

        rd = {}
        rd['companyname'] = ''
        rd['clientsxmlrows'] = ''
        rd['projectsxmlrows'] = ''
        rd['projectlocationsxmlrows'] = ''
        rd['peoplexmlrows'] = ''
        rd['projectpeoplexmlrows'] = ''

        docxml = "<projectnotes>\n"

        self.get_resource_groups(rgroups)

        self.getprojectsxml(rgroups, clientsdict, rd, parameter)

        docxml += "<table name=\"clients\">\n"
        for k in clientsdict:
            docxml +=  "  <row>\n   <column name=\"client_name\">" + self.pnc.to_xml(k) + "</column>\n </row>\n"
        docxml +=  "</table>\n"

        docxml += "<table name=\"people\">\n"
        docxml += rd['peoplexmlrows']
        docxml += "</table>\n"

        docxml += "<table name=\"projects\">\n"
        docxml += rd['projectsxmlrows']
        docxml += "</table>\n"

        docxml += "<table name=\"project_people\">\n"
        docxml += rd['projectpeoplexmlrows']
        docxml += "</table>\n"

        docxml += "<table name=\"project_locations\">\n"
        docxml += rd['projectlocationsxmlrows']
        docxml += "</table>\n"

        docxml += "</projectnotes>\n"

        projectnotes.update_data(docxml)

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")

    def export_ifs_project_tracker_items(self, project_id, project_number):
        print(f"Gathering tracker items for {project_number}...")

        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table filter_field_1="project_id" filter_value_1="{project_id}" name="item_tracker" />\n</projectnotes>\n'
        xmlresult = projectnotes.get_data(xmldoc)

        #print(f"Tracker search returned: {xmlresult}")
        
        xmlval = QDomDocument()
        xmlval.setContent(xmlresult)
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node  

        # show all tracker items
        trackeritems = self.pnc.find_node(xmlroot, "table", "name", "item_tracker")

        isinternal = 0
        itemcount = 0
        itemstatus = ""
        itemtype = ""

        if not trackeritems is None:
            itemrow = trackeritems.firstChild()


            # find the ISSUES Activity
            jsact = self.get_open_activities(project_number)
            issuesseq = self.get_issues_activity(jsact)

            if not issuesseq is None:           
                itemrow = trackeritems.firstChild()

                while not itemrow.isNull():
                    isinternal = self.pnc.get_column_value(itemrow, "internal_item")
                    itemstatus = self.pnc.get_column_value(itemrow, "status")
                    itemtype = self.pnc.get_column_value(itemrow, "item_type")
                    ifsitemid = project_number + self.pnc.get_column_value(itemrow, "item_number")

                    print(f"Identified Issue {ifsitemid} in project {project_number}")

                    duedate = None
                    assignedto = ''
                    dateupdated = None
                    dateresolved = None
                    identifiedby = ''
                    priority = ''
                    status = ''

                    if self.pnc.get_column_value(itemrow, "date_due") is not None:
                        duedate = QDateTime.fromString(self.pnc.get_column_value(itemrow, "date_due"),'MM/dd/yyyy')

                    if self.pnc.get_column_value(itemrow, "last_update") is not None:
                        dateupdated = QDateTime.fromString(self.pnc.get_column_value(itemrow, "last_update"),'MM/dd/yyyy')

                    if self.pnc.get_column_value(itemrow, "date_resolved") is not None:
                        dateresolved = QDateTime.fromString(self.pnc.get_column_value(itemrow, "date_resolved"),'MM/dd/yyyy')

                    if self.pnc.get_column_value(itemrow, "assigned_to") is not None:
                        colnode = self.pnc.find_node(itemrow, "column", "name", "assigned_to")
                        if colnode.attributes().namedItem("lookupvalue").nodeValue() is not None and colnode.attributes().namedItem("lookupvalue").nodeValue() != '':
                            assignedto = colnode.attributes().namedItem("lookupvalue").nodeValue()

                    if self.pnc.get_column_value(itemrow, "identified_by") is not None:
                        colnode = self.pnc.find_node(itemrow, "column", "name", "identified_by")
                        if colnode.attributes().namedItem("lookupvalue").nodeValue() is not None and colnode.attributes().namedItem("lookupvalue").nodeValue() != '':
                            identifiedby = colnode.attributes().namedItem("lookupvalue").nodeValue()

                    priority = self.pnc.get_column_value(itemrow, "priority")
                    desc = self.pnc.get_column_value(itemrow, "description")
                    status = self.pnc.get_column_value(itemrow, "status")

                    if (isinternal != "1" and itemtype == "Tracker" and (itemstatus == "Assigned" or itemstatus == "New")):
                        if not self.update_activity_task(issuesseq, project_number, ifsitemid, self.pnc.get_column_value(itemrow, "item_name"), desc, assignedto, dateupdated, duedate, dateresolved, identifiedby, priority, status):
                            self.create_activity_task(issuesseq, project_number, ifsitemid, self.pnc.get_column_value(itemrow, "item_name"), desc, assignedto, dateupdated, duedate, dateresolved, identifiedby, priority, status)
                    else:
                        self.delete_activity_task(issuesseq, project_number, ifsitemid)    
                    
                    itemcount = itemcount + 1

                    itemrow = itemrow.nextSibling()

    # IMPORTANT:  This plugin relys on custom fields on the TASK Entity.  You'll need to evaluate the REST calls to determine the field info
    def export_ifs_tracker_items(self, parameter):
        timer = QElapsedTimer()
        timer.start()

        saved_state = None
        statename = "ifs_tracker_export"
        skip = 0
        top = 10

        projectcount = 0

        if parameter == "all":
            top = 300
        else:
            skip = self.pnc.get_save_state(statename)

        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table filter_field_1="project_status" filter_value_1="Active" name="projects" {self.pnc.state_range_attrib(top, skip)} />\n</projectnotes>\n'
        xmlresult = projectnotes.get_data(xmldoc)
        
        xmlval = QDomDocument()
        xmlval.setContent(xmlresult)
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node  

        # find all projects
        projects = self.pnc.find_node(xmlroot, "table", "name", "projects")

        if not projects is None:
            projectrow = projects.firstChild()

            while not projectrow.isNull():
                project_number = self.pnc.get_column_value(projectrow, "project_number")
                project_id = self.pnc.get_column_value(projectrow, "project_id")
                
                projectcount += 1

                self.export_ifs_project_tracker_items(project_id, project_number)

                projectrow = projectrow.nextSibling()

        self.pnc.set_save_state(statename, skip, top, projectcount)

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")


from includes.common import ProjectNotesCommon
from PyQt6 import QtCore
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo
from urllib3.exceptions import InsecureRequestWarning 

import requests
import json
from urllib3.exceptions import InsecureRequestWarning 

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

    def getearnedvaluemetrics(self, projectid):
            request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectMonitoringHandling.svc/Projects(ProjectId=%27' + projectid + '%27)/ProjectAnalysisArray?$apply=aggregate(Bcws%20with%20sum%20as%20Bcws_aggr_,Bcwp%20with%20sum%20as%20Bcwp_aggr_,Acwp%20with%20sum%20as%20Acwp_aggr_,Bac%20with%20sum%20as%20Bac_aggr_,Etc%20with%20sum%20as%20Etc_aggr_,Eac%20with%20sum%20as%20Eac_aggr_,Vac%20with%20sum%20as%20Vac_aggr_,Cv%20with%20sum%20as%20Cv_aggr_,Sv%20with%20sum%20as%20Sv_aggr_)'

            result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

            if (result.status_code != 200):
                QMessageBox.critical(None, "ODATA Request Failed", result.reason + ": " + result.text + " " + request_url, QMessageBox.StandardButton.Ok)
                print("ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
                return ""

            json_result = result.json()

            return(json_result['value'][0])

    def getcostmetrics(self, projectid):
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectMonitoringHandling.svc/Projects(ProjectId=%27' + projectid + '%27)/ProjectCostArray?$apply=aggregate(Estimated%20with%20sum%20as%20Estimated_aggr_,Planned%20with%20sum%20as%20Planned_aggr_,Baseline%20with%20sum%20as%20Baseline_aggr_,EarnedValue%20with%20sum%20as%20EarnedValue_aggr_,ScheduledWork%20with%20sum%20as%20ScheduledWork_aggr_,PlannedCommitted%20with%20sum%20as%20PlannedCommitted_aggr_,Committed%20with%20sum%20as%20Committed_aggr_,Used%20with%20sum%20as%20Used_aggr_,Actual%20with%20sum%20as%20Actual_aggr_)'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            QMessageBox.critical(None, "ODATA Request Failed", result.reason + ": " + result.text + " " + request_url, QMessageBox.StandardButton.Ok)
            print("ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return ""

        json_result = result.json()

        return(json_result['value'][0])

    def getopenactivities(self, projectid):
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ActivityListHandling.svc/Activities?$filter=((((Objstate%20eq%20IfsApp.ActivityListHandling.ActivityState%27Planned%27%20or%20Objstate%20eq%20IfsApp.ActivityListHandling.ActivityState%27Released%27))%20and%20(ProjectId%20eq%20%27' + projectid + '%27)))&$orderby=ShortName,ActivityNo&$select=ProjectId,Description,EarlyStartDate,EarlyFinishDate,Manager,Company,CCusPoSeq,ActivitySeq,Objstate,Objgrants,ProgressCost,ProgressHours,ActivityNo,ProgressTemplate,SubProjectId,AccessOn,EarlyStart,ActualStart,EarlyFinish,ActualFinish,TotalWorkDays,ShortName,ProgressMethod,ExcludeResourceProgress,ManualProgressLevel,ManualProgressCost,ManualProgressHours,EstimatedProgress,ProgressTemplateStep,PlannedCostDriver,Note,LateStart,LateFinish,luname,keyref&$expand=ProjectRef($select=Name,Objgrants,luname,keyref),SubProjectRef($select=Description,Objgrants,luname,keyref)'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            QMessageBox.critical(None, "ODATA Request Failed", result.reason + ": " + result.text + " " + request_url, QMessageBox.StandardButton.Ok)
            print("ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return ""

        json_result = result.json()

        return(json_result)

    def getactivitytasks(self, activityseq):
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectScopeAndScheduleHandling.svc/Activities(ActivitySeq=' + str(activityseq) + ')/ActivityTasks?$orderby=TaskId&$select=TaskId,Name,Info,Completed,Objgrants,CompletedDate,luname,keyref'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            QMessageBox.critical(None, "ODATA Request Failed", result.reason + ": " + result.text + " " + request_url, QMessageBox.StandardButton.Ok)
            print("ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return ""

        json_result = result.json()

        return(json_result)
        
    def getstatusitems(self, project_num, json_data, dayspan):
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

    def createactivitytask(self, activityseq, projectid, taskid, name, description, assignedto, dateupdated, datedue, dateresolved, identifiedby, priority, status):
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

        request_url = self.ifs_url + "/main/ifsapplications/projection/v1/CreateActivityTask.svc/TaskSet"

        result = requests.post(request_url, verify=False, auth=(self.ifs_username, self.ifs_password), json=docdata, headers={"Content-Type": "application/json"})
        
        if (result.status_code != 201):
            # print("CREATE ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)

            # print("Debug JSON: createactivitytask")
            # json_result = result.json()
            # print(json.dumps(json_result, indent=4))

            return False

        return True

    def updateactivitytask(self, activityseq, projectid, taskid, name, description, assignedto, dateupdated, datedue, dateresolved, identifiedby, priority, status):
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

        result = requests.patch(request_url, verify=False, auth=(self.ifs_username, self.ifs_password), json=docdata, headers={"Content-Type": "application/json"})
        
        if (result.status_code != 200):
            #print("UPDATE ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return False

        return True

    def deleteactivitytask(self, activityseq, projectid, taskid):
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

        result = requests.delete(request_url, verify=False, auth=(self.ifs_username, self.ifs_password), json=docdata, headers={"Content-Type": "application/json", "If-Match": "*"})
        
        if (result.status_code != 204):
            #print("DELETE ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return False

        return True

    def getissuesactivity(self, json_data):
        if 'value' in json_data:
            for j in json_data['value']:
                if 'ActivitySeq' in j:
                    if j['ActivityNo'] == 'ISSUES':      
                        return j['ActivitySeq']

        return None

    def downloadreport(self, reporturl, savelocation):
        result = requests.get(reporturl,  auth=(self.domain_user, self.domain_password))

        # print("pepareing to download: " + reporturl)
        # print("to: " + savelocation)

        if (result.status_code != 200):
            QMessageBox.critical(None, "File Download Failed", result.reason + ": " + result.text, QMessageBox.StandardButton.Ok)
            print("File Download Failed", result.reason + ": " + result.text)
            return False

        QFile.remove(savelocation) 
        with open(savelocation, mode="wb") as file:
            file.write(result.content)

        return True

    def getprojectsxml(self, rgroups, clientsdict, rd):
        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectsHandling.svc/Projects?$filter=(((Objstate%20eq%20IfsApp.ProjectsHandling.ProjectState%27Initialized%27%20or%20Objstate%20eq%20IfsApp.ProjectsHandling.ProjectState%27Started%27%20or%20Objstate%20eq%20IfsApp.ProjectsHandling.ProjectState%27Approved%27))%20and%20Manager%20eq%20%27' + self.ifs_person_id + '%27)&$select=BudgetControlOn,ControlAsBudgeted,ControlOnTotalBudget,ProjUniquePurchase,ProjUniqueSale,State,ProjectId,Objstate,Objgrants,Name,Company,CustomerCategory,CustomerId,FinancialProjectExist,History,DefaultSite,ProjectPngExists,CheckForecast,Description,CompanyName,Manager,AccessOnOff,PlanStart,PlanFinish,ActualStart,ActualFinish,ApprovedDate,CloseDate,CancelDate,FrozenDate,EarnedValueMethod,BaselineRevisionNumber,Cf_Lastinvoiced,luname,keyref&$expand=AccountingProjectRef($select=ProjectGroup,Objgrants,luname,keyref),ManagerRef($select=Name,luname,keyref),CustomerIdRef($select=Name,Objgrants,luname,keyref)'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            QMessageBox.critical(None, "ODATA Request Failed", result.reason + ": " + result.text + " " + request_url, QMessageBox.StandardButton.Ok)
            print("ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return ""

        json_result = result.json()

        #print("Debug JSON:")
        #print(json.dumps(json_result, indent=4))

        for rowval in json_result['value']:
            rd['companyname'] = rowval['CompanyName']

            rd['projectsxmlrows'] = rd['projectsxmlrows']  + "  <row>\n"

            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"project_number\">" + self.pnc.to_xml(rowval['ProjectId']) + "</column>\n"
            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"project_name\">" + self.pnc.to_xml(rowval['Description']) + "</column>\n"
            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"last_invoice_date\">" + self.pnc.to_xml(rowval['Cf_Lastinvoiced']) + "</column>\n"

            if rowval['CustomerIdRef'] is not None:
                rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"client_id\" lookupvalue=\"" + self.pnc.to_xml(rowval['CustomerIdRef']['Name']) + "\"></column>\n"
                
                # only add the company name once to the client list
                clientsdict[rowval['CustomerIdRef']['Name']] = True

            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"project_status\">Active</column>\n"

            metrics = ifsc.getearnedvaluemetrics(rowval['ProjectId'])
            costmetrics = ifsc.getcostmetrics(rowval['ProjectId'])

            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"budget\">" + str(costmetrics['Baseline_aggr_']) + "</column>\n"
            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"actual\">" + str(costmetrics['Used_aggr_']) + "</column>\n"
            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"bcwp\">" + str(costmetrics['EarnedValue_aggr_']) + "</column>\n"
            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"bcws\">" + str(costmetrics['ScheduledWork_aggr_']) + "</column>\n"
            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "    <column name=\"bac\">" + str(metrics['Bac_aggr_']) + "</column>\n"
            rd['projectsxmlrows']  = rd['projectsxmlrows']  + "  </row>\n"

            rd['projectlocationsxmlrows'] = rd['projectlocationsxmlrows'] + self.pnc.find_projectlocations( rowval['ProjectId'], ProjectsFolder)

            getteammembersxml(rgroups, clientsdict, rowval['ProjectId'], rd )
            
    def getresourcegroups(rgroups):
        request_url = IFSUrl + '/main/ifsapplications/projection/v1/ResourceGroupsHandling.svc/ResourceSet?$select=ResourceId,Description'

        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Content-Type" : "application/json"})

        if (result.status_code != 200):
            QMessageBox.critical(None, "ODATA Request Failed", result.reason + ": " + result.text + " " + request_url, QMessageBox.StandardButton.Ok)
            print("ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return ""

        #print(result.content)

        json_result = result.json()

        #print("Debug JSON:")
        #print(json.dumps(json_result, indent=4))

        for value in json_result['value']:
            rgroups[value['ResourceId']] = value['Description']        


    def getteammembersxml(rgroups, clientsdict, projectid, rd):
        request_url = IFSUrl + '/main/ifsapplications/projection/v1/ProjectResourcePlanningHandling.svc/ProjectSet(ProjectId=%27' + projectid + '%27)/ProjectAllocationArray?$apply=groupby((EmployeeName,ResourceId))'
        
        result = requests.get(request_url, verify=False, auth=(self.ifs_username, self.ifs_password),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            QMessageBox.critical(None, "ODATA Request Failed", result.reason + ": " + result.text + " " + request_url, QMessageBox.StandardButton.Ok)
            print("ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return ""

        json_result = result.json()

        #print("Debug JSON:")
        #print(json.dumps(json_result, indent=4))

        for rowval in json_result['value']:

            rd['projectpeoplexmlrows'] = rd['projectpeoplexmlrows'] + "  <row>\n"

            rd['projectpeoplexmlrows'] = rd['projectpeoplexmlrows'] + "    <column name=\"project_id\" lookupvalue=\"" + self.pnc.to_xml(projectid) + "\"></column>\n"
            rd['projectpeoplexmlrows'] = rd['projectpeoplexmlrows'] + "    <column name=\"people_id\" lookupvalue=\"" + self.pnc.to_xml(rowval['EmployeeName']) + "\"></column>\n"
            rd['projectpeoplexmlrows'] = rd['projectpeoplexmlrows'] + "    <column name=\"role\">" + self.pnc.to_xml(rgroups[rowval['ResourceId']]) + "</column>\n"
            rd['projectpeoplexmlrows'] = rd['projectpeoplexmlrows'] + "  </row>\n"

            rd['peoplexmlrows'] = rd['peoplexmlrows'] + "  <row>\n"

            rd['peoplexmlrows'] = rd['peoplexmlrows'] + "    <column name=\"name\">" + self.pnc.to_xml(rowval['EmployeeName']) + "</column>\n"
            rd['peoplexmlrows'] = rd['peoplexmlrows'] + "    <column name=\"client_id\" lookupvalue=\"" + self.pnc.to_xml(rd['companyname']) + "\"></column>\n"

            rd['peoplexmlrows'] = rd['peoplexmlrows'] + "  </row>\n"

            # only add the company name once to the client list
            clientsdict[rd['companyname']] = True


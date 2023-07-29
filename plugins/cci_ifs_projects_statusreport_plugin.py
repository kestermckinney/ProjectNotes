
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt5 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt5.QtSql import QSqlDatabase
from PyQt5.QtXml import QDomDocument, QDomNode
from PyQt5.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo
from PyQt5.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt5.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Generate Status Report"
plugindescription = "Generate a status report based on the options selected."
plugintable = "projects" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

# events must have a data structure and data view specified
#
# Structures:
#      string          The event will pass a python string containing XML and will expect the plugin to return an XML string
#
# Data Views:
#      clients
#      people
#      projects
#      project_people
#      status_report_items
#      project_locations
#      project_notes
#      meeting_attendees
#      item_tracker_updates
#      item_tracker

# Supported Events

# def event_startup(xmlstr):
#     return ""
#
# def event_shutdown(xmlstr):
#     return ""
#
# def event_everyminute(xmlstr):
#     return ""
#
# def event_every5minutes(xmlstr):
#     return ""
#
# def event_every10minutes(xmlstr):
#     return ""
#
# def event_every30Mmnutes(xmlstr):
#     return ""
#
# def event_menuclick(xmlstr):
#     return ""

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = [
]

OracleUsername = ""
ProjectsFolder = ""

# make global to pass between sections
mtotalcost = 0
mtotalinvoiced = 0
mtotalpoline = 0
mtotalinvoicable = 0

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()
    pne = ProjectNotesExcelTools()

    def populate_tande(sheet, adodb, projectnumber):
        strsql = """
        Select project_id, po_number, po_line_number, pcr_number, po_amount, sum(invoiceable_amount) as invoiceable
        from
    	(
        select
        cpo.project_id, p.description, cpo.po_no as po_number, cpo.po_line as po_line_number, cpo.chg_ord_no as pcr_number, cpo.amount as po_amount, 'Milestone' as type,
        NVL((select sum(ip.sales_amount) from NORT1APP.invoicing_plan ip where ip.project_id = cpo.project_id and cpo.po_no=ip.c_po_no and ip.c_po_line = cpo.po_line and ip.c_chg_ord_no  = cpo.chg_ord_no and ip.invoice_id is not null and ip.invoicability = 'Invoiceable') ,0) as invoiceable_amount
        from NORT1APP.c_customer_p_o cpo join NORT1APP.project p on	cpo.project_id=p.project_id	where p.project_id='"""
        strsql += projectnumber
        strsql += """'
        union all
    	select
    	cpo.project_id, p.description, cpo.po_no as po_number, cpo.po_line as po_line_number, cpo.chg_ord_no as pcr_number, cpo.amount as po_amount, 'Time' as type,
    	NVL((select sum(pt.sales_amount) from NORT1APP.project_transaction pt where pt.project_id = cpo.project_id and cpo.po_no=pt.c_po_no and pt.c_po_line = cpo.po_line and pt.c_chg_ord_no = cpo.chg_ord_no and pt.invoicability    = 'Invoiceable' and pt.report_cost_type = 'Time'), 0) as invoiceable_amount
    	from NORT1APP.c_customer_p_o cpo join NORT1APP.project p on	cpo.project_id=p.project_id	where p.project_id='"""
        strsql += projectnumber
        strsql += """'
    	union all
    	select
    	cpo.project_id, p.description, cpo.po_no as po_number, cpo.po_line as po_line_number, cpo.chg_ord_no as pcr_number, cpo.amount as po_amount, 'Expense' as type,
    	NVL(( select sum(pt.sales_amount) from NORT1APP.project_transaction pt where pt.project_id = cpo.project_id and pt.c_po_line = cpo.po_line and pt.c_chg_ord_no = cpo.chg_ord_no and pt.invoicability = 'Invoiceable' and pt.report_cost_type = 'Cost'), 0) as invoiceable_amount
    	from NORT1APP.c_customer_p_o cpo join NORT1APP.project p on	cpo.project_id=p.project_id	where p.project_id='"""
        strsql += projectnumber
        strsql += """'
    	)
        group by  project_id, po_number, po_line_number, pcr_number, po_amount
        order by
      	po_number, po_line_number, pcr_number
        """

        #print("T and E SQL:  " + strsql + "\n")
        recordset = win32com.client.Dispatch("ADODB.Recordset")

        recordset.Open(strsql, adodb)

        count = 0

        # get a count first
        while not recordset.EOF:
            count = count + 1
            recordset.MoveNext()

        if count > 0:
            recordset.MoveFirst()

        pne.expand_row(sheet, "<PONUMBER>", count)

        count = 0

        mtotalpoline = 0
        mtotalinvoicable = 0

        while not recordset.EOF:
            count = count + 1

            pne.replace_cell_tag(sheet, "<PONUMBER" + str(count) + ">", recordset.Fields.Item("po_number").Value )
            pne.replace_cell_tag(sheet, "<POLINE" + str(count) + ">", recordset.Fields.Item("po_line_number").Value )
            pne.replace_cell_tag(sheet, "<PCR" + str(count) + ">", recordset.Fields.Item("pcr_number").Value )

            pne.replace_cell_tag(sheet, "<POAMOUNT" + str(count) + ">", recordset.Fields.Item("po_amount").Value )
            mtotalpoline = mtotalpoline + float(recordset.Fields.Item("po_amount").Value.replace(',','') )

            pne.replace_cell_tag(sheet, "<POINVOICABLE" + str(count) + ">", recordset.Fields.Item("invoiceable").Value )
            mtotalinvoicable = mtotalinvoicable + float(recordset.Fields.Item("invoiceable").Value.replace(',','') )

            #print("invoicable " + str(mtotalinvoicable))
            #print("po line total " + str(mtotalpoline))

            recordset.MoveNext()

        recordset.Close()

        recordset = None

        if mtotalpoline == 0:
            pne.replace_cell_tag(sheet, "<PERCENTCONSUMED>", "0")
            #print("po line zero")
        else:
            pne.replace_cell_tag(sheet, "<PERCENTCONSUMED>", "" + str(  mtotalinvoicable / mtotalpoline ) )
            #print("calcs are " +  str(  mtotalinvoicable / mtotalpoline ) )

        pne.replace_cell_tag(sheet, "<TOTALPOS>", str(mtotalpoline))
        pne.replace_cell_tag(sheet, "<TOTALINVOICABLE>", str(mtotalinvoicable))

        if mtotalpoline == 0:
            pne.cliprowsbytags(sheet, "<TETOP>", "<TEBOTTOM>")
        else:
            pne.replace_cell_tag(sheet, "<TETOP>", "")
            pne.replace_cell_tag(sheet, "<TEBOTTOM>", "")

    def populate_milestones(sheet, adodb, projectnumber):
        strsql = """select PROJECT_ID, NORT1APP.ACTIVITY_API.Get_Description(ACTIVITY_SEQ) MILESTONENAME, SALES_AMOUNT, ACCOUNT_DATE,
        INVOICABILITY, INVOICE_ID, NORT1APP.PROJECT_INVOICE_API.Get_INVOICE_NO(COMPANY,INVOICE_ID),
        NORT1APP.INVOICING_PLAN_API.Get_Invoice_Status(INVOICING_PLAN_SEQ) STATUS, INVOICING_PLAN_CORRECTION,
        C_PO_NO, C_PO_LINE, C_CHG_ORD_NO , SHORT_NAME
        from NORT1APP.INVOICING_PLAN where project_id='"""
        strsql += projectnumber
        strsql += """' and (invoicing_plan_correction not in ('Correction', 'Corrected') or invoicing_plan_correction is null) order by short_name"""

        recordset = win32com.client.Dispatch("ADODB.Recordset")

        recordset.Open(strsql, adodb)
        #print("Opening milestone recordset")

        count = 0

        # get a count first
        while not recordset.EOF:
            count = count + 1
            recordset.MoveNext()

        if count > 0:
            recordset.MoveFirst()

        pne.expand_row(sheet, "<MILESTONENAME>", count)

        count = 0

        mtotalcost = 0
        mtotalinvoiced = 0

        while not recordset.EOF:
            count = count + 1

            pne.replace_cell_tag(sheet, "<MILESTONENAME" + str(count) + ">", recordset.Fields.Item("MILESTONENAME").Value)
            pne.replace_cell_tag(sheet, "<MILESTONECOST" + str(count) + ">", recordset.Fields.Item("SALES_AMOUNT").Value)
            mtotalcost = mtotalcost + float(recordset.Fields.Item("SALES_AMOUNT").Value.replace(',',''))

            pne.replace_cell_tag(sheet, "<MILESTONESTATUS" + str(count) + ">", recordset.Fields.Item("STATUS").Value)

            if not recordset.Fields.Item("STATUS").Value == "Notinvoiced":
                pne.replace_cell_tag(sheet, "<MILESTONEINVOICED" + str(count) + ">", recordset.Fields.Item("SALES_AMOUNT").Value)
                mtotalinvoiced = mtotalinvoiced + float(recordset.Fields.Item("SALES_AMOUNT").Value.replace(',',''))
            else:
                pne.replace_cell_tag(sheet, "<MILESTONEINVOICED" + str(count) + ">", "")

            recordset.MoveNext()

        recordset.Close()
        recordset = None

        pne.replace_cell_tag(sheet, "<MILESTONECOSTTOTAL>", str(mtotalcost))
        pne.replace_cell_tag(sheet, "<MILESTONEINVOICEDTOTAL>", "" + str(mtotalinvoiced))

        if mtotalcost == 0:
            pne.cliprowsbytags(sheet, "<MILESTONETOP>", "<MILESTONEBOTTOM>")
        else:
            pne.replace_cell_tag(sheet, "<MILESTONETOP>", "")
            pne.replace_cell_tag(sheet, "<MILESTONEBOTTOM>", "")

    def populate_financials(xmldoc, sheet, adodb, projectnumber, financialsdate):
        global OracleUsername
        global ProjectsFolder

        strsql = """select p.PROJECT_ID, p.DESCRIPTION, u.customer_id, u.name, CONTROL_CATEGORY, COST_ELEMENT_DESCRIPTION, ESTIMATED, PLANNED, BASELINE, EARNED_VALUE, SCHEDULED_WORK, PLANNED_COMMITTED, COMMITTED, USED, ACTUAL,
        TO_CHAR((select max(invoice_date) from NORT1APP.INVOICE i where i.project_id=d.project_id), 'MM/DD/YYYY') as last_invoice,
        ( ACTUAL + (BASELINE - EARNED_VALUE) / (EARNED_VALUE / ACTUAL) * (EARNED_VALUE / SCHEDULED_WORK) ) AS EAC,
        (  (ACTUAL - EARNED_VALUE)  / EARNED_VALUE ) AS CV,
        ( (EARNED_VALUE - SCHEDULED_WORK) / SCHEDULED_WORK ) AS SV,
        ( EARNED_VALUE / BASELINE ) AS PCT,
        ( EARNED_VALUE / ACTUAL ) AS CPI
        from NORT1APP.PROJ_CON_DET_SUM_COST_PROJECT d right join
        NORT1APP.project p on d.project_id=p.project_id right join
        NORT1APP.customer_info u on p.customer_id=u.customer_id
        where (d.CONTROL_CATEGORY='LABOR-INT' or p.STATE='Initialized' or p.STATE='Approved') and  p.manager='"""
        strsql += OracleUsername.upper()
        strsql += "' and p.state not in ('Closed', 'Completed')"

        if not projectnumber == "" and not projectnumber == None:
            strsql = strsql + " and p.project_id ='" + projectnumber + "'"

        #print(strsql + "\n")

        recordset = win32com.client.Dispatch("ADODB.Recordset")

        recordset.Open(strsql, adodb)
        #print("Opened Recordset...")

        table = pnc.xml_table(xmldoc, "projects")
        #print("Created table xml...")

        while not recordset.EOF:
            #print("processing projects row....")

            row = pnc.xml_row(xmldoc)
            table.appendChild(row)

            row.appendChild(pnc.xml_col(xmldoc, "project_number",recordset.Fields.Item("project_id").Value, None))
            row.appendChild(pnc.xml_col(xmldoc, "project_name", recordset.Fields.Item("description").Value, None))
            row.appendChild(pnc.xml_col(xmldoc, "last_status_date",pnc.to_xml(financialsdate), None))
            row.appendChild(pnc.xml_col(xmldoc, "last_invoice_date",recordset.Fields.Item("last_invoice").Value, None))
            row.appendChild(pnc.xml_col(xmldoc, "budget",pnc.to_xml(recordset.Fields.Item("estimated").Value) , None))
            row.appendChild(pnc.xml_col(xmldoc, "actual",pnc.to_xml(recordset.Fields.Item("actual").Value) , None))
            row.appendChild(pnc.xml_col(xmldoc, "bcwp" ,pnc.to_xml(recordset.Fields.Item("earned_value").Value) , None))
            row.appendChild(pnc.xml_col(xmldoc, "bcws",pnc.to_xml(recordset.Fields.Item("scheduled_work").Value), None))
            row.appendChild(pnc.xml_col(xmldoc, "bac",pnc.to_xml(recordset.Fields.Item("baseline").Value), None))

            row.appendChild(pnc.xml_col(xmldoc, "client_id", None,pnc.to_xml(recordset.Fields.Item("name").Value)))

            #row.appendChild(pnc.xml_col("project_status", "Active", None))

            # populate excel fields
            pne.replace_cell_tag(sheet, "<PROJECTNUMBER>", recordset.Fields.Item("project_id").Value)
            pne.replace_cell_tag(sheet, "<DESCRIPTION>", recordset.Fields.Item("description").Value)
            pne.replace_cell_tag(sheet, "<PROJECTNUMBER>", recordset.Fields.Item("project_id").Value)

            pne.replace_cell_tag(sheet, "<BCWS>", recordset.Fields.Item("SCHEDULED_WORK").Value)
            pne.replace_cell_tag(sheet, "<BCWP>", recordset.Fields.Item("EARNED_VALUE").Value)
            pne.replace_cell_tag(sheet, "<BAC>", recordset.Fields.Item("baseline").Value)
            pne.replace_cell_tag(sheet, "<ACTUAL>", recordset.Fields.Item("actual").Value)
            pne.replace_cell_tag(sheet, "<EAC>", recordset.Fields.Item("EAC").Value)
            pne.replace_cell_tag(sheet, "<CV>", recordset.Fields.Item("CV").Value)
            pne.replace_cell_tag(sheet, "<SV>", recordset.Fields.Item("SV").Value)
            pne.replace_cell_tag(sheet, "<PERCENTCOMPLETE>", recordset.Fields.Item("PCT").Value)
            pne.replace_cell_tag(sheet, "<CPI>", recordset.Fields.Item("CPI").Value)

            # get the milestones
            recordset.MoveNext()

        recordset.Close()
        recordset = None

        return(table)

    # processing main function
    def event_data_rightclick(xmlstr):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
            return ""

        if not pnc.verify_global_settings():
            return("")

        emaillist = ""

        # setup global variables
        global OracleUsername
        global ProjectsFolder
        OracleUsername = pnc.get_global_setting("OracleUsername")
        ProjectsFolder = pnc.get_global_setting("ProjectsFolder")

        statusdate = QDateTime.currentDateTime()
        executedate = statusdate
        fullreport = False
        keepexcel = False
        emailashtml = False
        emailaspdf = False
        emailasexcel = False
        noemail = False

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()   

        ui = uic.loadUi("plugins/includes/dialogStatusRptOptions.ui")
        ui.m_datePickerRptDateStatus.setCalendarPopup(True)
        ui.setWindowFlags(
            QtCore.Qt.Window |
            QtCore.Qt.WindowCloseButtonHint |
            QtCore.Qt.WindowStaysOnTopHint
            )

        ui.m_datePickerRptDateStatus.setDateTime(statusdate)
        ui.m_checkBoxDisplayStatus.setChecked(False)
        ui.m_radioBoxEmailAsHTML.setChecked(True)

        if ui.exec() == QDialog.Accepted:
            fullreport = ui.m_checkBoxFullRptStatus.isChecked()
            statusdate = ui.m_datePickerRptDateStatus.dateTime()
            keepexcel = ui.m_checkBoxExcelRptStatus.isChecked()

            emailashtml = ui.m_radioBoxEmailAsHTML.isChecked()
            emailaspdf = ui.m_radioBoxEmailAsPDF.isChecked()
            emailasexcel = ui.m_radioBoxEmailAsExcel.isChecked()
            noemail = ui.m_radioBoxDoNotEmail.isChecked()

            QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            QtWidgets.QApplication.processEvents()        
        else:
            QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            QtWidgets.QApplication.processEvents()
            return("")

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()

        if (projectfolder is None or projectfolder ==""):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return("")
        else:
            projectfolder = projectfolder + "\\Status Reports\\"

        progbar = QProgressDialog()
        progbar.setWindowTitle("Generating Report...")
        progbar.setWindowFlags(
            QtCore.Qt.Window |
            QtCore.Qt.WindowCloseButtonHint |
            QtCore.Qt.WindowStaysOnTopHint
            )

        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()
        progval = 0
        progtot = 11

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")
        period = pnc.get_column_value(projtab.firstChild(), "status_report_period")

        email = None
        nm = None
        stat = None
        receivers = ""

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("looking up recipients...")
        QtWidgets.QApplication.processEvents()   

        teammember = pnc.find_node(xmlroot, "table", "name", "project_people")
        if not teammember.isNull():
            memberrow = teammember.firstChild()

            while not memberrow.isNull():
                nm = pnc.get_column_value(memberrow, "name")
                email = pnc.get_column_value(memberrow, "email")
                stat = pnc.get_column_value(memberrow, "receive_status_report")
                if not nm == pm:
                    if (not email == None and not email == "" and stat == "1"):
                        if not receivers == "":
                            receivers = receivers + ", "
                            emaillist = emaillist + ";"

                        receivers = receivers + nm
                        emaillist = emaillist + email


                memberrow = memberrow.nextSibling()

            progval = progval + 1
            progbar.setValue(int(min(progval / progtot * 100, 100)))
            progbar.setLabelText("Copying files...")
            QtWidgets.QApplication.processEvents()   

        fullreportname = projectfolder + statusdate.toString("yyyyMMdd ") + projnum + " Status Report Full.xlsx"
        fullreportpdf = projectfolder + statusdate.toString("yyyyMMdd ") + projnum + " Status Report Full.pdf"
        basereportname = projectfolder + statusdate.toString("yyyyMMdd ") + projnum + " Status Report.xlsx"
        basereportpdf = projectfolder + statusdate.toString("yyyyMMdd ") + projnum + " Status Report.pdf"

        QFile.copy("plugins/templates/Status Report Template.xlsx", fullreportname)

        handle = pne.open_excel_document(fullreportname)
        sheet = handle['workbook'].Sheets("Status Report")

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Gathering status items...")
        QtWidgets.QApplication.processEvents()   

        # count expand out excell rows for status report items
        repitem = pnc.find_node(xmlroot, "table", "name", "status_report_items")
        compcount = 0
        progcount = 0
        category = None
        description = None

        #print("looking for status report items...")
        if not repitem is None:
            #print("found status report rows...")
            repitemrow = repitem.firstChild()

            while not repitemrow.isNull():
                #print("found an item to count...")
                category = pnc.get_column_value(repitemrow, "task_category")
                description = pnc.get_column_value(repitemrow, "task_description")

                if category == "In Progress":
                    progcount = progcount + 1
                elif category == "Completed":
                    compcount = compcount + 1

                repitemrow = repitemrow.nextSibling()

            pne.expand_row(sheet, "<INPROGRESS>", progcount)
            pne.expand_row(sheet, "<COMPLETED>", compcount)

            progval = progval + 1
            progbar.setValue(int(min(progval / progtot * 100, 100)))
            progbar.setLabelText("Gathering status items...")
            QtWidgets.QApplication.processEvents()   

            compcount = 0
            progcount = 0
            repitemrow = repitem.firstChild()

            while not repitemrow.isNull():
                #print("replacing status items cell value..")
                category = pnc.get_column_value(repitemrow, "task_category")
                description = pnc.get_column_value(repitemrow, "task_description")

                if category == "In Progress":
                    progcount = progcount + 1
                    pne.replace_cell_tag(sheet, "<INPROGRESS" + str(progcount) + ">", description)
                elif category == "Completed":
                    compcount = compcount + 1
                    pne.replace_cell_tag(sheet, "<COMPLETED" + str(compcount) + ">", description)

                repitemrow = repitemrow.nextSibling()

        progval = progval + 1
        #print("going to gather tracker....")
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Gathering tracker items...")
        QtWidgets.QApplication.processEvents()   
        #print("gathering tracker items...")

        # show all tracker items
        trackeritems = pnc.find_node_by2(xmlroot, "table", "name", "item_tracker", "filter_field_1", "project_id")
        #print("returned from find by node")
        itemcount = 0
        isinternal = 0
        itemstatus = ""
        itemtype = ""

        if not trackeritems is None:
            #print("found tracker items...")
            itemrow = trackeritems.firstChild()

            while not itemrow.isNull():
                isinternal = pnc.get_column_value(itemrow, "internal_item")
                itemstatus = pnc.get_column_value(itemrow, "status")
                itemtype = pnc.get_column_value(itemrow, "item_type")

                #print("internal: " + isinternal + " status: " + itemstatus + " item type: " + itemtype + " item number: " +  pnc.get_column_value(itemrow, "item_number"))

                if (isinternal != "1" and itemtype == "Tracker" and (itemstatus == "Assigned" or itemstatus == "New")):
                    ##print("counting tracker items..")
                    itemcount = itemcount + 1

                itemrow = itemrow.nextSibling()

            pne.expand_row(sheet, "<ISSUENAME>", itemcount)

            itemcount = 0

            itemrow = trackeritems.firstChild()

            while not itemrow.isNull():
                isinternal = pnc.get_column_value(itemrow, "internal_item")
                itemstatus = pnc.get_column_value(itemrow, "status")
                itemtype = pnc.get_column_value(itemrow, "item_type")

                if (isinternal != "1" and itemtype == "Tracker" and (itemstatus == "Assigned" or itemstatus == "New")):
                    itemcount = itemcount + 1

                #print("populating tracker items name: " + pnc.get_column_value(itemrow, "item_name"))

                pne.replace_cell_tag(sheet, "<ISSUENAME" + str(itemcount) + ">", pnc.get_column_value(itemrow, "item_name"))
                pne.replace_cell_tag(sheet, "<ISSUEIDENTIFIED" + str(itemcount) + ">", pnc.get_column_value(itemrow, "date_identified"))
                pne.replace_cell_tag(sheet, "<ISSUEPRIORITY" + str(itemcount) + ">", pnc.get_column_value(itemrow, "priority"))
                pne.replace_cell_tag(sheet, "<ISSUEDUEDATE" + str(itemcount) + ">", pnc.get_column_value(itemrow, "date_due"))
                pne.replace_cell_tag(sheet, "<ISSUESTATUS" + str(itemcount) + ">", itemstatus)

                itemrow = itemrow.nextSibling()

        # don't show task items
        if itemcount == 0:
            pne.cliprowsbytags(sheet, "<ISSUESTOP>", "<ISSUESBOTTOM>")

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Pulling IFS project summary...")
        QtWidgets.QApplication.processEvents()   

        docxml = QDomDocument()
        docroot = docxml.createElement("projectnotes");
        docxml.appendChild(docroot);

        adodb = pnc.connect()

        node = populate_financials(docxml, sheet, adodb, projnum, statusdate.toString("MM/dd/yyyy"))
        docroot.appendChild(node)

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Pulling IFS milestones...")
        QtWidgets.QApplication.processEvents()   

        #print("Calling populate milestones")
        populate_milestones(sheet, adodb, projnum)

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Pulling IFS PO information...")
        QtWidgets.QApplication.processEvents()   

        #print("Calling populate T & E")
        populate_tande(sheet, adodb, projnum)

        #pne.replace_cell_tag(sheet, "<STATUSDATE>", executedate.toString("MM/dd/yyyy"))
        pne.replace_cell_tag(sheet, "<ENDINGDATE>", statusdate.toString("MM/dd/yyyy"))
        pne.replace_cell_tag(sheet, "<REPORTINGPERIOD>", period )
        pne.replace_cell_tag(sheet, "<PROJECTMANAGER>", pm)
        pne.replace_cell_tag(sheet, "<RECEIVERS>", receivers)

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Pulling IFS project summary...")
        QtWidgets.QApplication.processEvents()   

        #ts = QDateTime()
        periodstart = statusdate
        if period == "Bi-Weekly":
            periodstart = periodstart.addDays(-14)
            #ts = -2
        elif period == "Weekly":
            periodstart = periodstart.addDays(-7)
            #ts = -1
        elif period == "Monthly":
            periodstart = periodstart.addMonths(-1)
            #ts:SetMonths(-1)

        #print("Pre Period Calcs Status Date: " + statusdate.toString("MM/dd/yyyy") + "\n")
        if not period == "None":
            pne.replace_cell_tag(sheet, "<STARTINGDATE>", periodstart.toString("MM/dd/yyyy"))
        else:
            pne.replace_cell_tag(sheet, "<STARTINGDATE>", "")

        #print("Post Period Calcs Status Date: " + statusdate.toString("MM/dd/yyyy") + "\n")

        pnc.close(adodb)

        # clear unused tags
        pne.replace_cell_tag(sheet, "<COMPLETED>", "")
        pne.replace_cell_tag(sheet, "<INPROGRESS>", "")
        pne.replace_cell_tag(sheet, "<ISSUESTOP>", "")
        pne.replace_cell_tag(sheet, "<ISSUESBOTTOM>", "")

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Finalizing Excel files...")
        QtWidgets.QApplication.processEvents()   

        if (progcount + compcount == 0):
            pne.cliprowsbytags(sheet, "<STATUSITEMTOP>", "<STATUSITEMBOTTOM>")
        else:
            pne.replace_cell_tag(sheet, "<STATUSITEMTOP>", "")
            pne.replace_cell_tag(sheet, "<STATUSITEMBOTTOM>", "")


        pne.replace_cell_tag(sheet, "<RUNDATE>", executedate.toString("MM/dd/yyyy"))
        handle['workbook'].Save()

        if not QFile.copy(fullreportname, basereportname):
            QMessageBox.critical(None, "Unable to copy template", "Could not copy " + fullreportname + " to " + basereportname, QMessageBox.Cancel)
            return ""

        handle2 = pne.open_excel_document(basereportname)
        sheet2 = handle2['workbook'].Sheets("Status Report")

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Finalizing Excel files...")
        QtWidgets.QApplication.processEvents()   

        pne.cliprowsbytags(sheet2, "<FULLREPORTTOP>", "<FULLREPORTBOTTOM>")
        pne.replace_cell_tag(sheet2, "<FULLREPORTTOP>", "")
        pne.replace_cell_tag(sheet2, "<FULLREPORTBOTTOM>", "")

        pne.replace_cell_tag(sheet, "<FULLREPORTTOP>", "")
        pne.replace_cell_tag(sheet, "<FULLREPORTBOTTOM>", "")

        handle['workbook'].Save()
        handle2['workbook'].Save()

        # generate PDFs
        if not fullreport is None:
            pne.save_excel_as_pdf(handle, sheet, fullreportpdf)

        pne.save_excel_as_pdf(handle2, sheet2, basereportpdf)
        # should we email?
        if noemail == False:
            subject = projnum + " " + projdes + " Status Report " + statusdate.toString("MM/dd/yyyy")
            #print("Ready to email")
            if emailashtml:
                pne.email_excel_html(sheet2, subject, emaillist, None)
            elif emailasexcel:
                pne.email_excel_html(sheet2, subject, emaillist, basereportname)
            elif emailaspdf:
                pne.email_excel_html(sheet2, subject, emaillist, basereportpdf)

        pne.close_excel_document(handle)
        pne.close_excel_document(handle2)
        #print("closing exel documents")

        if keepexcel == False:
            QFile.remove(fullreportname)
            QFile.remove(basereportname)
        elif fullreport == False:
            QFile.remove(fullreportname)
        #print("removing extra files")

        if ui.m_checkBoxDisplayStatus.isChecked():
            if fullreport:
                QDesktopServices.openUrl(QUrl("file:///" + fullreportpdf, QUrl.TolerantMode))
            else:
                QDesktopServices.openUrl(QUrl("file:///" + basereportpdf, QUrl.TolerantMode))

        progbar.setValue(100)
        progbar.setLabelText("Finalizing Excel files...")
        QtWidgets.QApplication.processEvents()   
        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed
        #print("closing progress bar...")

        pne.killexcelautomation()

        return(docxml.toString())

# setup test data
"""
import sys
print("Buld up QDomDocument")
app = QApplication(sys.argv)

xmldoc = QDomDocument("TestDocument")
f = QFile("C:/Users/pamcki/Desktop/project.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

print(event_data_rightclick(xmldoc.toString()))
"""


# TODO: Right click returned XML isn't getting imported
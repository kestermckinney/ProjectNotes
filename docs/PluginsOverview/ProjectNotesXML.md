# Project Notes XML

## Understanding Project Notes XML Format

Nearly all information found in Project Notes can be exported or imported using XML. The best way to understand the XML format is to export it. The format is based around a tree structure described below. XML files can be used to easily exchange information with other Project Notes users. The Python plugin system uses the XML structure to exchange information between Project Notes and the plugin architecture. Information on how to export a file can be found in [The File Menu](<../InterfaceOverview/FileMenu.md>) section.

**The XML format:**

```xml
<?xml version="1.0" encoding="UTF-8"?>
<projectnotes filepath="C:UsersMyAccountOneDriveProject NotesMyDatabase.db" export_date="09/22/2020 12:38 PM" filter_field="project_id" project_manager_id="159709810500028597" managing_company_id="{ba96fb89-6c2d-46db-864c-5be6292b1045}" managing_company_name="My Company, Inc." managing_manager_name="My Name" filter_values="{ba96fb89-6c2d-46db-864c-5be6292b10ef}">
 <table name="ix_projects" filter_field_1="project_id" filter_value_1="159709812400019208">
  <row id="{ba96fb89-6c2d-46db-864c-5be6292b10e4}">
   <column name="project_number">P4000</column>
   <column name="project_name">IT New Server Install</column>
   <column name="last_status_date">09/02/2020</column>
   <column name="last_invoice_date">09/02/2020</column>
   <column name="primary_contact" lookupvalue="Jim Smith">{ba96fb89-6c2d-46db-864c-5be6292b10e4}</column>
   <column name="budget">$2090.00</column>
   <column name="actual">$649.00</column>
   <column name="bcwp">$649.00</column>
   <column name="bcws">$2090.00</column>
   <column name="bac">$2090.00</column>
   <column name="invoicing_period">Monthly</column>
   <column name="status_report_period">None</column>
   <column name="client_id" lookupvalue="Special Law Firm">{ba96fb89-6c2d-46db-864c-5be6292b10gg}</column>
   <column name="project_status">Active</column>
   <table name="ix_status_report_items" filter_field="project_id" filter_value="159709812400019208"/>
   <table name="ix_project_people" filter_field="project_id" filter_value="{ba96fb89-6c2d-46db-864c-5be6292b10ef}">
    <row id="{ba96fb89-6c2d-46db-884c-5be6292b10eo}">
     <column name="project_id" lookupvalue="P4000">{ba96fb89-6c2d-46db-864c-5be6292b10ef}</column>
     <column name="project_id_name" lookupvalue="IT New Server Install">159709812400019208</column>
     <column name="people_id" lookupvalue="Mike Smith">{7e6df350-ab03-4653-99f1-7abb09bbefa3}</column>
     <column name="role"></column>
     <column name="receive_status_report"></column>
     <column name="name">Mike Smith</column>
     <column name="email">Mike.Smith@mycompany.com</column>
     <column name="client_name">My Company, Inc.</column>
    </row>
    <row id="159907543300015593">
     <column name="project_id" lookupvalue="J2627">{7e6df350-ab03-4653-99f1-7abb09bbefa3}</column>
     <column name="project_id_name" lookupvalue="IT New Server Install">159709812400019208</column>
     <column name="people_id" lookupvalue="Jim Smith">{92ce1e4e-a908-4f6c-8348-c85beb459942}</column>
     <column name="role">Lead Support</column>
     <column name="receive_status_report">1</column>
     <column name="name">Jim Smith</column>
     <column name="email">jim_smith@speciallawfirm.com</column>
     <column name="client_name">Speical Law Firm</column>
    </row>
   </table>
   <table name="ix_project_locations" filter_field="project_id" filter_value="{7e6df350-ab03-4653-99f1-7abb09bbefa3}">
    <row id="{7e6df350-ab03-4653-99f1-7abb09bbefa3}">
     <column name="project_id" lookupvalue="P4000">{56601f6b-30d9-4fc0-a7e1-d367d30a4e52}</column>
     <column name="location_type">PDF File</column>
     <column name="location_description">Quote : Project Quote.pdf</column>
     <column name="full_path">P:ProjectFolderP4000_IT New Server InstallQuotesQT9876908.pdf</column>
     <column name="project_id_name" lookupvalue="IT New Server Install">{3d9f88d9-810b-4e6c-827b-9afeef0dd48f}</column>
    </row>
   </table>
   <table name="ix_project_notes" filter_field="project_id" filter_value="{3d9f88d9-810b-4e6c-827b-9afeef0dd48f}"/>
   <table name="ix_item_tracker" filter_field="project_id" filter_value="159709812400019208"/>
  </row>
 </table>
</projectnotes>
```

The XML document follows a parent child heiarchy that corresponds to the record relationships found in your database. If a &lt;row> has several related records it will contain one or more &lt;table> nodes as children. Data in the XML export is often repeated to make plugin writing easier. Below is a table that describes the different node and attribute types.

| **Element** | **Type** | **Description** | **Required For Import** |
| --- | --- | --- | :---: |
| &lt;projectnotes\> | Document Root Node |  | Yes |
| filepath | Attribute | The database the XML was exported from | No |
| export\_date | Attribute | The date the data was exported | No |
| filter\_field_# | Attribute | The field filtered to limit the data exported.  This used when calling get_data to find results.  Multiple field value pairs can be used as long as # matches. | No |
| filter\_value_# | Attribute | The value used to filter the data exported   This used when calling get_data to find results.  Multiple field value pairs can be used as long as # matches.|  |
| project\_manager\_id | Attribute | The database record id of the Project Manager specified in the Preferences | No |
| managing\_company\_id | Attribute | The database record id of the Managing Company specified in the Preferences | No |
| managing\_company\_name | Attribute | The name of the Project Manager specified in the Preferences | No |
| managing\_manager\_name | Attribute | The name of the Managing Company specified in the Preferences | No |
| &lt;row\> | Document Node | The database row exported or imported | Yes |
| id | Attribute | The record id of the record, typically not included on an import. Project Notes generates a globally unique identifier for each record. If specified, the import process will update a specific record. It is important to realize exporting and id from one database into another will cause errors. The receiving database may have similar data with completely different ids. ids are intended to be globally unique when the record is crearted. | No |
| &lt;column\> | Document Node | The column node to be imported. The value may be the column value, or the lookupvalue may be provided to specify the import lookup the corresponding record id. Typically corresponding record ids are not given as values. | Yes |
| name | Attribute | The name of the column to be imported | Yes |
| lookupvalue | Attribute | In many cases a record relates to another record. For example People and Clients, are related by their record id. To relate records on import, this value should contain the corresponding name. For example: The project\_id field should have this value set to the project\_number. The client\_id field should have this value set to the client name. The people\_id should have this field set to the people name. | Yes |

<br>
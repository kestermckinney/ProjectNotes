# Project Notes XML

# Understanding Project Notes XML Format

Nearly all information found in Project Notes can be exported or imported using XML.&nbsp; The best way to understand the XML format is to export it. The format is based around a tree structure described below.&nbsp; XML files can be used to easily exchange information with other Project Notes users.&nbsp; The Lua plugin system uses the XML structure to exchange information between Project Notes and the plugin architecture.&nbsp; Information on how to export a file can be found in [The File Menu](<FileMenu.md>) section.

&nbsp;

**The XML format:**

\
\<?xml version="1.0" encoding="UTF-8"?\>\
\<**projectnotes** filepath="**C:\\Users\\MyAccount\\OneDrive\\Project Notes\\MyDatabase.db**" export\_date="**09/22/2020 12:38 PM**" filter\_field="**project\_id**" project\_manager\_id="**159709810500028597**" managing\_company\_id="**{ba96fb89-6c2d-46db-864c-5be6292b1045}**" managing\_company\_name="**My Company, Inc.**" managing\_manager\_name="**My Name**" filter\_values="**{ba96fb89-6c2d-46db-864c-5be6292b10ef}**"\>\
**&nbsp;** \<**table** name="**ix\_projects**" filter\_field="**project\_id**" filter\_value="**159709812400019208**"\>\
**&nbsp;** &nbsp; \<**row** id="**{ba96fb89-6c2d-46db-864c-5be6292b10e4}**" delete="true"\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**project\_number**"\>**P4000**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**project\_name**"\>**IT New Server Install**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**last\_status\_date**"\>**09/02/2020**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**last\_invoice\_date**"\>**09/02/2020**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**primary\_contact**" lookupvalue="**Jim Smith**"\>**{ba96fb89-6c2d-46db-864c-5be6292b10e4}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**budget**"\>**$2090.00**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**actual**"\>**$649.00**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**bcwp**"\>**$649.00**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**bcws**"\>**$2090.00**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**bac**"\>**$2090.00**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**invoicing\_period**"\>**Monthly**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**status\_report\_period**"\>**None**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**client\_id**" lookupvalue="**Special Law Firm**"\>**{ba96fb89-6c2d-46db-864c-5be6292b10gg}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**project\_status**"\>**Active**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**table** name="**ix\_status\_report\_items**" filter\_field="**project\_id**" filter\_value="**159709812400019208**"/\>\
**&nbsp;** &nbsp; &nbsp; \<**table** name="**ix\_project\_people**" filter\_field="**project\_id**" filter\_value="**{ba96fb89-6c2d-46db-864c-5be6292b10ef}**"\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; \<**row** id="**{ba96fb89-6c2d-46db-884c-5be6292b10eo}**"\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**project\_id**" lookupvalue="**P4000**"\>**{ba96fb89-6c2d-46db-864c-5be6292b10ef}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**project\_id\_name**" lookupvalue="**IT New Server Install**"\>**159709812400019208**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**people\_id**" lookupvalue="**Mike Smith**"\>**{7e6df350-ab03-4653-99f1-7abb09bbefa3}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**role**"\>\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**receive\_status\_report**"\>\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**name**"\>**Mike Smith**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**email**"\>**Mike.Smith@mycompany.com**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**client\_name**"\>**My Company, Inc.**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; \</**row**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; \<**row** id="**159907543300015593**"\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**project\_id**" lookupvalue="**J2627**"\>**{7e6df350-ab03-4653-99f1-7abb09bbefa3}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**project\_id\_name**" lookupvalue="**IT New Server Install**"\>**159709812400019208**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**people\_id**" lookupvalue="**Jim Smith**"\>**{92ce1e4e-a908-4f6c-8348-c85beb459942}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**role**"\>**Lead Support**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**receive\_status\_report**"\>**1**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**name**"\>**Jim Smith**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**email**"\>**jim\_smith@speciallawfirm.com**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**client\_name**"\>**Speical Law Firm**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; \</**row**\>\
**&nbsp;** &nbsp; &nbsp; \</**table**\>\
**&nbsp;** &nbsp; &nbsp; \<**table** name="**ix\_project\_locations**" filter\_field="**project\_id**" filter\_value="**{7e6df350-ab03-4653-99f1-7abb09bbefa3}**"\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; \<**row** id="**{7e6df350-ab03-4653-99f1-7abb09bbefa3}**"\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**project\_id**" lookupvalue="**P4000**"\>**{56601f6b-30d9-4fc0-a7e1-d367d30a4e52}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**location\_type**"\>**PDF File**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**location\_description**"\>**Quote : Project Quote.pdf**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**full\_path**"\>**P:\\ProjectFolder\\P4000\_IT New Server Install\\Quotes\\QT9876908.pdf**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; &nbsp; \<**column** name="**project\_id\_name**" lookupvalue="**IT New Server Install**"\>**{3d9f88d9-810b-4e6c-827b-9afeef0dd48f}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; &nbsp; \</**row**\>\
**&nbsp;** &nbsp; &nbsp; \</**table**\>\
**&nbsp;** &nbsp; &nbsp; \<**table** name="**ix\_project\_notes**" filter\_field="**project\_id**" filter\_value="**{3d9f88d9-810b-4e6c-827b-9afeef0dd48f}**"/\>\
**&nbsp;** &nbsp; &nbsp; \<**table** name="**ix\_item\_tracker**" filter\_field="**project\_id**" filter\_value="**159709812400019208**"/\>\
**&nbsp;** &nbsp; \</**row**\>\
**&nbsp;** \</**table**\>\
\</**projectnotes**\>

&nbsp;

The XML document follows a parent child heiarchy that corresponds to the record relationships found in your database.&nbsp; If a \<row\> has several related records it will contain one or more \<table\> nodes as children.&nbsp; Data in the XML export is often repeated to make plugin writing easier.&nbsp; Below is a table that describes the different node and attribute types.

&nbsp;

| **Element** | **Type** | **Description** | **Required For Import** |
| --- | --- | --- | :---: |
| \<projectnotes\> | Document Root Node | &nbsp; | Yes |
| filepath | Attribute | The database the XML was exported from | No |
| export\_date | Attribute | The date the data was exported | No |
| filter\_field | Attribute | The field filtered to limit the data exported | No |
| filter\_value | Attribute | The value used to filter the data exported | &nbsp; |
| project\_manager\_id | Attribute | The database record id of the Project Manager specified in the Preferences | No |
| managing\_company\_id | Attribute | The database record id of the Managing Company specified in the Preferences | No |
| managing\_company\_name | Attribute | The name of the Project Manager specified in the Preferences | No |
| managing\_manager\_name | Attribute | The name of the Managing Company specified in the Preferences | No |
| \<row\> | Document Node | The database row exported or imported | Yes |
| id | Attribute | The record id of the record, typically not included on an import.&nbsp; Project Notes generates a globally unique identifier for each record.&nbsp; If specified, the import process will update a specific record.&nbsp; It is important to realize exporting and id from one database into another will cause errors.&nbsp; The receiving database may have similar data with completely different ids.&nbsp; ids are intended to be globally unique when the record is crearted. | No |
| delete | Attribute | Based on the information specified the import will attempt to delete the specified row.&nbsp; Be aware deleting a parent record will leave the child records abandon. | No |
| \<column\> | Document Node | The column node to be imported.&nbsp; The value may be the column value, or the lookupvalue may be provided to specify the import lookup the corresponding record id.&nbsp; Typically corresponding record ids are not given as values. | Yes |
| name | Attribute | The name of the column to be imported | Yes |
| lookupvalue | Attribute | In many cases a record relates to another record.&nbsp; For example People and Clients, are related by their record id.&nbsp; To relate records on import, this value should contain the corresponding name.&nbsp; For example:&nbsp; The project\_id field should have this value set to the project\_number.&nbsp; The client\_id field should have this value set to the client name.&nbsp; The people\_id should have this field set to the people name. | Yes |


&nbsp;


***
_Created with the Personal Edition of HelpNDoc: [Save time and frustration with HelpNDoc's WinHelp HLP to CHM conversion feature](<https://www.helpndoc.com/step-by-step-guides/how-to-convert-a-hlp-winhelp-help-file-to-a-chm-html-help-help-file/>)_

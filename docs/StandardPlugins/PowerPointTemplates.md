# Power Point Templates (Windows Only)

## Quickly Creating a Project Power Point

You can quickly fill out key project values in a Power Point Document related. The **Get PowerPoint Template** copies the selected PowerPoint template to the project meeting minutes folder and fills out basic project information. The new document is added to the list of project artifacts found in the **File/Folder Locations** list in the [Project Information Page](<../InterfaceOverview/ProjectPage.md>).

## PowerPoint File Names and Locations

The plugin looks for a **File/Folder Location** with a description of ***"Project Folder"*** to determine where to save the generated document . See [Project Information Page](<../InterfaceOverview/ProjectPage.md>) to learn about **File/Folder Location**. Files are placed in the sub-folder **"Meeting Minutes"** found in the specified ***"Project Folder"***. The file name generated has the word Template removed from the original name.

The **Get PowerPoint Template** file is generated by copying the selected file found in **"Project Notes\\plugins\\templates"** to the **"Project Folder\\Meeting Minutes"**. By using OLE Automation for Microsoft PowerPoint ([OLE Automation Wiki](<https://en.wikipedia.org/wiki/OLE\_Automation>)), the plugin opens and edits the template file filling in values. Because the plugin is automating the activity of an outside application unpredictable situations may occur. You will need to save all edits and generate a PDF manually.

## Tag Values Populated by This Plugin

| **Tag** | **Description** |
| :--- | :--- |
| &lt;PROJECTNUMBER> | This tag is replaced by the project number. |
| &lt;PROJECTNAME> | This tag is replaced by the full project name. |

<br>
**To create a PowerPoint document from a template:**

1. From [Project List Page](<../InterfaceOverview/ProjectListPage.md>), right-click on the Project.
2. Choose **Get PowerPoint Template**.
3. Select a PowerPoint file listed from the **"Project Notes\\plugins\\templates"** folder.
4. Make **Microsoft PowerPoint** the active application with Alt+Tab or click the **Microsoft PowerPoint** icon in the task bar.
5. Finish editing the document.
6. Choose **Save**, from the **File Menu** in **Microsoft PowerPoint**.
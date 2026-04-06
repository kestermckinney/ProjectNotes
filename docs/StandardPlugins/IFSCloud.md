# IFS Cloud Integration

Project Notes includes optional integration with the **IFS Cloud ERP** system. The integration uses basic authentication (username and password) to communicate with the IFS REST API.

## IFS Cloud Settings

**To open the IFS Cloud settings:**

1. From the **Plugins** menu choose **Settings > IFS Cloud Settings**.

| Setting | Default | Description |
| :--- | :--- | :--- |
| **Username** | *(none)* | Your IFS Cloud username for API authentication. |
| **Password** | *(none)* | Your IFS Cloud password. |
| **URL** | `https://ifs.cornerstonecontrols.com` | The base URL of your IFS Cloud instance. Change this to match your organization's IFS deployment. |
| **Person ID** | *(none)* | Your IFS Person ID, used to correlate Project Notes data with IFS records. |
| **Domain User** | *(none)* | Windows domain username (used for integrated Windows authentication on some deployments). |
| **Domain Password** | *(none)* | Windows domain password. |
| **Report Server** | `indvifsbi05` | The hostname of the SQL Server Reporting Services (SSRS) report server. Change this to match your organization's report server. |
| **Sync Tracker Items** | *(unchecked)* | When checked, tracker items will be synchronized with IFS work orders or tasks during the background sync. |

## IFS Project Synchronization

The **IFS Integration** background thread runs automatically while Project Notes is open. It periodically imports project data from IFS Cloud into Project Notes.

**To trigger an immediate IFS project import:**

1. From the **Plugins** menu choose **Utilities > Import IFS Projects**.

When **Sync Tracker Items** is enabled in IFS Cloud Settings, tracker and action items are also pushed to IFS as work orders or tasks during each sync cycle.

## SSRS Report Generation (Windows Only)

The **SSRS Report Capture** plugin retrieves reports from a **SQL Server Reporting Services** server and saves them as PDFs. This is useful for pulling standard IFS reports (e.g., project cost or schedule reports) directly into your project folder.

The plugin looks for a **File/Folder Location** with the description **"Project Folder"** to determine where to save the generated report. The default output sub-folder is `Project Management/Status Reports`. You can change this sub-folder under **Plugins > Settings > SQL Report**.

**To generate an SSRS report:**

1. Right-click on a project in the **Project List Page**.
2. Choose **Generate Status Report**.
3. The plugin connects to the report server using the credentials configured in **IFS Cloud Settings** and downloads the report.
4. The PDF is saved to `[Project Folder]/Project Management/Status Reports/`.

## Authentication

IFS Cloud integration uses **basic authentication** only. OAuth and token-based authentication are not supported. Credentials are stored in the local application settings.

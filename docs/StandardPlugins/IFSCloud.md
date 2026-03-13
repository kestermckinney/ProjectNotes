# IFS Cloud Integration

Project Notes includes optional integration with the **IFS Cloud ERP** system. The integration uses basic authentication (username and password) to communicate with the IFS REST API.

## IFS Cloud Settings

**To open the IFS Cloud settings:**

1. From the **Plugins** menu choose **Settings > IFS Cloud Settings**.

| Setting | Description |
| :--- | :--- |
| **Username** | Your IFS Cloud username for API authentication. |
| **Password** | Your IFS Cloud password. |
| **URL** | The base URL of your IFS Cloud instance (e.g., `https://yourcompany.ifs.cloud`). |
| **Person ID** | Your IFS Person ID, used to correlate Project Notes data with IFS records. |
| **Domain User** | Windows domain username (used for integrated Windows authentication on some deployments). |
| **Domain Password** | Windows domain password. |
| **Report Server** | The URL of the SQL Server Reporting Services (SSRS) report server, if using SSRS report generation. |
| **Sync Tracker Items** | When checked, tracker items will be synchronized with IFS work orders or tasks. |

## SSRS Report Generation

The **SSRS Report Capture** plugin can retrieve reports from a **SQL Server Reporting Services** server and save them as PDFs. This is useful for pulling standard IFS reports (e.g., project cost reports or schedule reports) directly into your project folder.

**To generate an SSRS report:**

1. Right-click on a project in the **Project List Page**.
2. Choose the SSRS report menu item configured in your setup.
3. The plugin connects to the report server using the credentials configured in **IFS Cloud Settings** and downloads the report.
4. The PDF is saved to the project folder.

## Authentication

IFS Cloud integration uses **basic authentication** only. OAuth and token-based authentication are not supported. Credentials are stored in the local application settings.

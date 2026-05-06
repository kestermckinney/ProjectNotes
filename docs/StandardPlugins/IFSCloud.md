# IFS Cloud Integration

Project Notes includes optional integration with the **IFS Cloud ERP** system. The integration uses **Keycloak SSO** (OAuth2 browser-redirect flow) to authenticate with the IFS REST API. The first time a sync runs, a browser window opens for you to sign in; the token is then cached and refreshed automatically.

## Installing the IFS Plugins

The IFS integration is shipped as an **optional component** of the Project Notes installer. On Windows, the installer presents a **"Custom IFS Plugins"** section you can deselect; on macOS, the installer offers an **"IFS Custom Plugins"** package.

The custom IFS plugins are written for the specific custom fields used in Cornerstone Controls' IFS deployment and will not work against an arbitrary IFS environment. Skip the IFS plugins component if your organization does not use these custom fields.

> **Note:** Installing the IFS plugins component modifies files inside the application bundle, which can invalidate the code signature on signed Project Notes packages. If signature integrity is required, install the IFS plugins to your user home folder instead, or run an unsigned build.

## IFS Cloud Settings

**To open the IFS Cloud settings:**

1. From the **Plugins** menu choose **Settings > IFS Cloud Settings**.

| Setting | Default | Description |
| :--- | :--- | :--- |
| **URL** | *(none)* | The base URL of your IFS Cloud instance (e.g., `https://ifs.cornerstonecontrols.com`). Used as both the REST API base URL and the Keycloak issuer. |
| **Realm** | `cciprod` | The Keycloak realm used for SSO. Change this to match the realm configured in your IFS deployment. |
| **Domain User** | *(none)* | Windows domain username used to authenticate with the SSRS report server (separate from IFS SSO). |
| **Domain Password** | *(none)* | Windows domain password for the report server. |
| **Report Server** | *(none)* | The hostname of the SQL Server Reporting Services (SSRS) report server. Change this to match your organization's report server. |
| **Sync Tracker Items** | *(unchecked)* | When checked, tracker items will be synchronized with IFS work orders or tasks during the background sync. |

Your IFS Person ID is determined automatically from the SSO token at sign-in — there is no longer a separate Person ID field.

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

IFS Cloud integration uses **Keycloak SSO** with the OAuth2 authorization-code flow. When the integration first needs a token, Project Notes opens your default browser to the IFS sign-in page; after you complete sign-in, the token is cached locally and refreshed silently as long as the refresh token is valid. If the cached token has expired and the browser flow cannot complete (for example, you don't respond), the integration backs off for a short cool-down before prompting again so it does not repeatedly open the browser.

A network connectivity check runs before each authentication attempt; if the IFS host is unreachable, sign-in is skipped until the next cycle.

The Domain User, Domain Password, and Report Server settings are used only by the SSRS report capture step and are independent of the IFS REST API authentication.

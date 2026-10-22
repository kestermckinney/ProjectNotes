# IFS Cloud Integration

The **IFS Cloud** plugin connects Project Notes to a customized IFS Cloud ERP system. It imports the projects you manage with their budgets and team members, keeps tracker items in step with IFS tasks, generates SSRS status and tracker reports, and adds menus that open IFS pages and reports.

The plugin is written for the custom fields of one IFS deployment (Cornerstone Controls) and will not work against an arbitrary IFS environment.

## Installing

The plugin is not part of the Project Notes installers. Install the `com.projectnotespro.ifs-cloud` `.pnplugin` file from **Settings > Plugins > Install**, review what it asks for, and approve it. It goes into your own plugins folder, so the signed application is never modified and upgrades keep it.

Settings from the Python IFS plugins carry over the first time it starts.

## Settings

Open **Settings > IFS Cloud**.

| Setting | Default | Description |
| :--- | :--- | :--- |
| **IFS Cloud address** | `https://ifs.cornerstonecontrols.com` | The base address of your IFS Cloud system. |
| **Sign-in realm** | `cciprod` | The Keycloak realm used for sign-in. |
| **Keep tracker items in step with IFS ISSUES tasks** | off | Syncs tracker items with the tasks of each project's ISSUES activity. |
| **Report server** | `indvifsbi05` | The SSRS report server's host name. |
| **Domain user**, **Domain password** | none | The Windows account used for the report server. The password is moved to your system's credential store when you save. |
| **Project subfolder for status reports** | `Project Management/Status Reports` | Where status reports are saved in the project folder. |
| **Project subfolder for tracker reports** | `Project Management/Issues List` | Where tracker reports are saved in the project folder. |

The plugin may reach only the hosts listed in its manifest. If your IFS address or report server differs, the plugin's provider has to add it and rebuild the package; Project Notes then asks you to approve it again.

## Signing in

The first time the plugin needs IFS, your browser opens the IFS sign-in page. Afterwards the token is kept in your system's credential store and refreshed without the browser. If you don't finish signing in, the plugin waits an hour before opening the browser again. Your IFS user is found from the account you sign in with.

## Importing projects

Every three minutes while Project Notes is open, the plugin imports the next few of your active IFS projects: name, customer, budget, actual cost, earned value and team members with their resource groups. Projects that are no longer active in IFS are closed in Project Notes. **Plugins · Utilities > Import IFS Projects** imports them all at once.

## Tracker items

With tracker sync on, each active project's items are compared with the tasks of its ISSUES activity after the import:

- A task created in IFS becomes item `IFS<task id>`.
- An item created in Project Notes becomes a task named after the project and item number. Internal items stay out of IFS.
- When both sides have the item, the one updated more recently wins.

## Status reports

Right-click a project and choose **Generate Status Report**. Choose the report date and how to send it: in the message body (Windows, using Word), as a PDF, as a Word document, or not at all. The plugin downloads the report from the report server with the domain account, saves the PDF (and a Word copy, if you ask) in the project's report folder, prepares the email for your review, and records the date as the project's last status date.

The email goes to the project's default status report audience, or to the team members marked to receive status reports when there is none.

## Tracker reports

Right-click a project and choose **Generate Tracker Report** to file and send the SSRS **Issues List** report. The choices match the status report, including the Windows-only option that puts the Word version of the report in the message body. The report date only names the file (`20261001 P4000 Tracker Report.pdf`) and dates the subject; it doesn't change the project's last status date.

The PDF and the optional Word copy go in the tracker report subfolder. The email goes to the project's default Tracker Items Report audience, falling back to the team members marked to receive status reports.

## IFS shortcuts

The plugin adds **IFS Cloud** menus to projects and contacts (Open Project, Open Gantt, Invoicing Plan, Open Time Entry and others) and a **Reports** menu of SSRS reports to projects. They are listed in the **Shortcuts** table at the bottom of **Settings > IFS Cloud**, where you can rename, change, add or remove them. The table works like the one in My Shortcuts.

A URL can use `{{ ifs.address }}` and `{{ ifs.reportServer }}` for the IFS address and report server, so changing either setting updates every shortcut that uses it. Clear one and its shortcuts disappear from the menus. Empty the table and it stays empty; the defaults come back only on a fresh install.

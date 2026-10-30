# IFS Cloud Integration

The **IFS Cloud** plugin connects Project Notes to a customized IFS Cloud ERP system. It imports the projects you manage with their budgets and team members, keeps tracker items in step with IFS tasks, generates SSRS status and tracker reports, and adds menus to My Shortcuts that open IFS pages and reports.

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

### How ISSUES sync works

Every three minutes, after the project import, the plugin walks your active projects. For each one it looks up the activity numbered `ISSUES` in IFS. A project without that activity is skipped. Otherwise the plugin reads every task under it and matches them to the project's tracker items.

Matching uses the task ID. A task whose ID starts with the project number came from Project Notes, and the rest of the ID is the item number, so task `P4000012` is item `012` of project `P4000`. Any other task is item `IFS` plus its task ID. If no item has that number but one has the same name, the plugin updates that item rather than create a duplicate.

When a task and an item match, the plugin compares the item's **Updated** date with the task's date-updated field:

| Comparison | Result |
| :--- | :--- |
| Project Notes is newer | The item is written to the IFS task. |
| Both dates are the same | Nothing changes. |
| IFS is newer, or either date is blank | The task overwrites the item. |

Tasks with no matching item become new items. Items with no matching task become new IFS tasks, except internal items and items whose number starts with `IFS`. Those are tasks someone deleted in IFS, and the plugin leaves them alone instead of recreating them.

Editing an item in Project Notes sets its **Updated** date to today. That date is what tells the plugin to send your change to IFS.

### Why there is no sync timestamp

The **Keep tracker items in step with IFS ISSUES tasks** setting is only a check box. Nothing records when it was turned on or when a sync last finished. Without that reference point the plugin can't tell what changed since the last sync. It only knows which side carries the later date. Expect these consequences:

- **First sync pushes everything.** Turn the setting on for a project with existing items and every non-internal item that IFS lacks becomes a new IFS task at once, including items closed long ago. Mark items internal first if you don't want them in IFS.
- **Same-day edits on both sides are lost.** Updated dates hold a day, not a time. If you edit an item on the day someone also edits its task in IFS, the dates match and neither change crosses over. The two stay different until one side is edited again on a later day.
- **One side's change can overwrite the other's.** Whichever side has the later date wins the whole item. Someone who edits the task in IFS today replaces the status you set in Project Notes yesterday, even if they changed a different field.
- **Deletions come back.** The plugin can't tell an item you deleted from an item that never existed. Delete an item in Project Notes while its task remains in IFS and the next sync brings it back. Delete a Project Notes task in IFS and the item is sent back as a new task. Delete the matching record on both sides, or close the item instead.
- **Turning the setting off and on again catches up blindly.** Changes made on both sides while sync was off are settled by date alone, with the same-day and whole-item problems above.
- **Blank dates favor IFS.** A task or item with no updated date is treated as older, so IFS overwrites the item.

If you depend on IFS and Project Notes agreeing, pick one side as the place you edit tracker items and treat the other as a copy.

## Status reports

Right-click a project and choose **Generate Status Report**. Choose the report date and how to send it: in the message body (Windows, using Word), as a PDF, as a Word document, or not at all. The plugin downloads the report from the report server with the domain account, saves the PDF (and a Word copy, if you ask) in the project's report folder, opens the email as a draft in your mail application, and records the date as the project's last status date.

### Choosing recipients

If you email the report, a **Status Report Recipients** dialog opens before anything is downloaded. It's the same recipient panel the built-in reports use. Tick the people to include, set each one to **To**, **Cc** or **Bcc**, search the list, or use **Select all**, **Clear** and **Reset recipients**. Then choose **Generate**. **Cancel** stops there, and nothing is downloaded or saved.

It starts with the project's default status report audience. Without one, it starts with the team members marked **Receive Status**, leaving out the project manager. The **Saved audience** dropdown, **Save audience** and **Set as report default** work as they do in the built-in reports. See [Saved audiences](<../InterfaceOverview/ReportsAndEmail.md#saved-audiences>).

The plugin and the built-in Status Report share the same audiences and the same default. Set a default in either one and the other loads it too.

Choosing **Do not email** skips the dialog.

## Tracker reports

Right-click a project and choose **Generate Tracker Report** to file and send the SSRS **Issues List** report. The choices match the status report, including the Windows-only option that puts the Word version of the report in the message body. The report date only names the file (`20261001 P4000 Tracker Report.pdf`) and dates the subject; it doesn't change the project's last status date.

The PDF and the optional Word copy go in the tracker report subfolder. Recipients are chosen the same way as for the status report. The panel starts with the project's default Tracker Items Report audience, a default it shares with the built-in Tracker Items Report.

## IFS shortcuts

The plugin puts **IFS Cloud** menus on projects and contacts (Open Project, Open Gantt, Invoicing Plan, Open Time Entry and others) and a **Reports** menu of SSRS reports on projects. They live in **Settings > My Shortcuts**, so the My Shortcuts plugin must be enabled for them to appear. Rename, change or remove them there.

Each shortcut is added once, with the IFS address and report server written into its URL. Delete one and it stays deleted. My Shortcuts keeps any entry it already had with the same menu, submenu and entity, such as the ones the Python plugin set up, and the IFS one is skipped. A Reports shortcut waits until the report server is filled in.

Changing the IFS address or report server later does not update shortcuts that were already added. Edit their URLs in My Shortcuts.

Before version 2.2 the shortcuts were kept in a table under **Settings > IFS Cloud**. On its first start, 2.2 moves your entries from that table, including any you changed, to My Shortcuts in place of the defaults.

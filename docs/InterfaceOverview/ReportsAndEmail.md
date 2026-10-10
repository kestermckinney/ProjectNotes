# Reports and Email

Project Notes builds communications from your saved project data:

- **Send Notes** for one saved meeting note
- **Meeting Notes Report**, **Status Report** and **Tracker Items Report** for a whole project
- **Project Email** for the project team or one contact attached to the project
- **Share Location** to email a link or a file attachment

Each one ends the same way. Project Notes prepares the content, you review it, and then it opens a draft in your mail application. Nothing leaves your machine until you press Send there.

Set the integration up once before using any of this. See [Email and Meeting Setup](<EmailAndMeetingSetup.md>).

## Save before you start

Reports are built from what is saved, not from what is on screen. Project Notes saves the open record for you when you start a report, but if a note or a tracker item is open and unsaved somewhere else, save it first.

## Sending one meeting note

1. Open the project note, or right-click its row in the project's **Notes** tab.
2. Choose **Send Notes**.
3. The **Send Notes** dialog opens with the note prepared. Meeting attendees are already checked as **To** recipients. Other team members appear unchecked and default to **Cc**.
4. Adjust the list, then click **Send Email**.

The notes are formatted into the body of the message. There is no attachment and no format choice.

## Project email and location sharing

Right-click a project in the sidebar, project list, or project page and choose **Project Email…** to start a message to its team. Right-click a team member or a meeting attendee and choose **Project Email…** to preselect that contact. Both actions use the **Project Email** subject and body configured in **Settings → Email Templates**; `contact.name` and `contact.email` refer to the contact clicked and are empty for a project-team message. The review offers the same recipient editing and saved-audience controls as report emails. Save or update an audience by name and optionally make it the project-email default. A contact action preserves the clicked recipient instead of automatically loading that default.

On the **Contacts** page or a contact's own page, choose **Email…** to start a blank message to that contact, even if they are not attached to a project. This action uses your configured email integration but does not apply a project template or offer project audiences.

In the project's **Locations** tab, right-click a location and choose **Share as Attachment…**. Web URLs also offer **Share Link…**. Local locations must point to an existing file to be attached. URL attachments are downloaded into the application cache before the review becomes ready; downloads have a 50 MB limit and a 60-second timeout. Download errors appear in the review and prevent creating an incomplete draft.

Location sharing uses the same saved audiences as reports. Choose an existing audience, adjust recipients, or save a new audience and optionally make it the sharing default. Configure the subject and body using the **Share Location** email type. Its fields include `location.description`, `location.url`, and `location.fileName`; keep `{{ content.body }}` in the body to include the link or attachment description.

Create Email Draft dialogs do not display the subject or email body. Project Email and Share Location drafts still use their configured templates; plain contact emails start blank. Adjust recipients and audiences where offered, then choose **Create Email Draft**. Review, edit, and send the message from your mail client. Cached downloads and attachment copies are deleted when Project Notes closes, so save or send the draft before quitting.

## Running a project report

1. Right-click the project, on the Projects list or on its own page.
2. Choose **Reports**, then **Meeting Notes…**, **Status…** or **Tracker Items…**.
3. The **Run report** dialog opens and starts preparing.

### Which report

The **Report** dropdown lets you switch between the three reports without closing the dialog. **Reporting date** sets the date printed on the report and available to templates as `report.date`.

**Generate internal report**, called **Include internal meetings** on the Meeting Notes Report, brings internal-only material into the output. Leave it off for anything a company will see. See [Presenting to Companies](<PresentingToCompanies.md>).

The Tracker Items Report adds two filter rows:

- **Include tracker items** — **Tracker**, **Action**, or both
- **Status** — **New**, **Assigned**, **Resolved**, **Deferred**, **Cancelled**

New and Assigned are on by default, which is the usual open-items list.

Changing any of these rebuilds the report immediately.

### What should happen with it

Two choices:

**Save the report** prepares the report and asks where to put it. Nothing is sent and no email client opens. The save dialog offers PDF or HTML, starting in your Documents folder with a filename built from the project number. Turn on **View report after save** in **Settings > Preferences** to have the saved file open once it is written.

**Email the report** creates a draft for the recipients you select.

### Email details

Pick a **Format**:

| Format | Result |
| :--- | :--- |
| **Inline HTML** | The report is the body of the email. Nothing to open, nothing to download. |
| **HTML attachment** | The email carries a standalone HTML file. |
| **PDF attachment** | The email carries a print-ready PDF. |

Then choose recipients under **Send to** and click **Send Email**.

With **Microsoft 365 drafts** selected as your integration, the draft is created in your mailbox and opened in Outlook on the web. With **iCloud**, the draft is created in iCloud Mail, but iCloud provides no link to the new item, so open Drafts to review it. With the other integrations, your mail application opens with the report and recipients already filled in.

## Choosing recipients

Recipients come from contact records attached to the project. A contact with no email address cannot be selected, and you cannot type an address into the dialog. To include someone new, add them as a contact and put them on the project team.

The recipient panel shows every candidate with the reason they qualify, such as **Receives status reports** or their team role. Above the list you get the running count and the unique **To**, **Cc** and **Bcc** totals. Each row has a checkbox and a role dropdown.

**Select all** and **Clear** work on the whole list. **Reset recipients** returns to the report's starting selection.

Without a saved default, the Status Report and the Tracker Items Report start with the team members marked **Receive Status** on the project's **Team** tab.

### Saved audiences

An audience is a named recipient selection stored on the project.

- Type a name in **Save audience preset** and click **Save audience** to store the current selection.
- Pick one from the **Saved audience** dropdown to load it. The checklist updates at once.
- **Set as report default** makes the chosen audience load automatically the next time you run this particular report on this project.

Audiences are shared across a project. One saved while running a Status Report is offered for the Tracker Items Report, the Meeting Notes Report and the **Schedule Meeting** dialog on the same project. Defaults are not shared: each report keeps its own, and the dropdown marks the current report's default.

Because meetings use **Required** and **Optional** rather than To, Cc and Bcc, an audience crossing between the two stores Required as To and Optional as Cc. A report's Bcc recipients load as Optional.

## Templates

The wording around each report comes from **Settings > Email Templates**. The generated report itself sits in a protected block that a template cannot drop or duplicate, so you can rewrite everything around it safely. See [Email and Meeting Setup](<EmailAndMeetingSetup.md>) for the editor and the field list.

## Temporary files

Rendered reports and staged attachments live in the application cache folder until Project Notes closes. Send or save an open draft before you quit, or its attachment may be gone when you come back to it.

## Related Documentation

- [Email and Meeting Setup](<EmailAndMeetingSetup.md>) — One-time setup for mail and calendar handoff
- [Scheduling Meetings](<ScheduleMeeting.md>) — Invitations from a project
- [Project Page](<ProjectPage.md>) — Team members, **Receive Status**, and status report items
- [Notes Page](<NotesPage.md>) — Writing the meeting notes these reports draw from
- [Presenting to Companies](<PresentingToCompanies.md>) — Keeping internal material out of company communications

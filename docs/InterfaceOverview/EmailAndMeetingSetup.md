# Email and Meeting Setup

Project Notes writes your project emails and meeting invitations for you, then hands them to the mail or calendar application you already use. It never sends anything itself. Every message and every invitation arrives as a draft that you read, adjust and send.

This page covers the one-time setup. Once it is done, see [Reports and Email](<ReportsAndEmail.md>) for sending reports and meeting notes, and [Scheduling Meetings](<ScheduleMeeting.md>) for invitations.

## What you need first

Four things must be in place before any email or invitation is worth sending.

| Requirement | Where to set it | Why it matters |
| :--- | :--- | :--- |
| **Project Manager** | **Settings > Preferences** | Identifies you. You are kept off recipient lists and become the organizer on meeting invitations. |
| **Managing Company** | **Settings > Preferences** | Separates your own people from client people, which drives internal reports and the client audience filters. |
| People with email addresses | **People** page | Recipients come only from people records. A person with a blank **Email** cannot be selected. |
| A project team | project's **Team** tab | Audiences are built from the team. Mark the people who should get status reports with **Receive Status**. |

You cannot type an address into a review dialog. If someone needs a copy, add them as a person and put them on the project team.

## Choose how mail and meetings leave Project Notes

Open **Settings > Preferences** and find **Email and Scheduling Integration**. One choice covers both email and meeting invitations.

| Integration | Email becomes | Meeting becomes | Works on |
| :--- | :--- | :--- | :--- |
| **Default Application** | A draft message file (`.eml`) opened by whatever application handles that file type | A calendar file (`.ics`) opened by your default calendar application | Windows, macOS, Linux |
| **Outlook** | A draft message opened in the new Outlook | A calendar file opened in the new Outlook, which leaves out the attendees. Copy them from the Schedule meeting window and paste them in | Windows |
| **Outlook (Classic)** | A draft message opened in Outlook (classic) | An unsent meeting window in Outlook (classic) with the attendees filled in | Windows |
| **MacOS Email and Calendar** | A Mail compose window with the formatted message. No Bcc and no attachments | An event created directly in Calendar, without attendees. Copy them from the Schedule meeting window and paste them in | macOS |
| **Thunderbird** | A Thunderbird compose window, attachments included | A calendar file handed to Thunderbird, which offers to import it | Windows, macOS, Linux |
| **Microsoft 365 drafts** | A draft in your Microsoft 365 mailbox, opened in Outlook on the web | A draft event in your Microsoft 365 calendar, opened in Outlook on the web | Windows, macOS, Linux |

The list only shows the choices your operating system supports, so no one sees all six at once:

- **Windows:** Default Application, Outlook, Outlook (Classic), Thunderbird and Microsoft 365 drafts.
- **macOS:** Default Application, MacOS Email and Calendar, Thunderbird and Microsoft 365 drafts.
- **Linux:** Default Application, Thunderbird and Microsoft 365 drafts.

On macOS, **MacOS Email and Calendar** is selected the first time you run Project Notes. Windows and Linux start on **Default Application**; on Windows, switch to the Outlook you use. Pick one, then follow the section for your platform below.

If your settings came from another computer and name a choice this one lacks, such as **Outlook (Classic)** on a Mac, Project Notes falls back to the platform default until you choose again.

## Windows with Outlook Classic

1. Open **Settings > Preferences > Email and Scheduling Integration** and set **Integration** to **Outlook (Classic)**.
2. Start Outlook and leave it running. Handoff is faster when Outlook is already open.

Project Notes starts Outlook (classic) itself, so the `.eml` and `.ics` file associations don't matter.

When you send a report, Project Notes writes the message with its attachments and opens it in Outlook as a new, unsent message. The subject, recipients, body and attachments are already filled in. Edit anything you like, then click **Send**.

When you schedule a meeting, Outlook opens an unsent meeting window. The attendees, time, location and agenda are already in it. Required attendees go on the **To** line and optional ones on the optional line. Check it over, then click **Send** to issue the invitations.

## Windows with the new Outlook

1. Set **Integration** to **Outlook**.
2. Sign in to the new Outlook with the account you send project mail from.

Project Notes opens the new Outlook directly, even when Outlook (classic) or another app holds the `.eml` and `.ics` associations.

Reports open as a draft message ready to send. Meetings are different. The new Outlook imports the calendar file as a plain event and drops the attendee list. Before you click **Open Invite**, click **Copy Attendees** in the Schedule meeting window. Then paste the attendees into the event in Outlook and send it.

If you want the meeting to arrive with its attendees already filled in, use **Outlook (Classic)** or **Microsoft 365 drafts** instead.

## Windows with another default app

**Default Application** opens the files with whatever app holds the `.eml` and `.ics` associations. Many calendar apps, the new Outlook among them, import an `.ics` file without its attendees. **Copy Attendees** is available with this choice too.

## macOS with Mail and Calendar

1. Set **Integration** to **MacOS Email and Calendar**.
2. Open Mail and Calendar at least once and add the account you send project mail from.
3. Schedule a test meeting. macOS asks whether Project Notes may access your calendars, then whether it may control Calendar. Allow both.

Reports open in a Mail compose window with the recipients and the formatted body already in place. Cc recipients appear alongside the To recipients. Bcc recipients and attachments cannot be carried and are left out. Choose **Default Application** for reports that need either.

Invitations become an event on your first writable calendar with the agenda in its notes. Calendar would send invitations the moment the event had attendees, so Project Notes adds none. Click **Copy Attendees** in the Schedule meeting window, paste the list into the event's invitees field, check the time, then click **Send**.

If you would rather work with files, choose **Default Application** instead. Mail must handle `.eml` files and Calendar must handle `.ics` files. To set that, select any such file in Finder, press **Command-I**, choose the application under **Open with**, then click **Change All**.

## Linux

1. Set **Integration** to **Default Application**.
2. Make sure your desktop has a mail application registered for `message/rfc822` and a calendar application registered for `text/calendar`. Most desktops set this when you install Evolution, Geary, Thunderbird or GNOME Calendar.

If your desktop has no handler for either type, choose **Thunderbird** instead and give Project Notes the path to it.

## Thunderbird on any platform

1. Set **Integration** to **Thunderbird**.
2. Fill in **Thunderbird executable**. A plain command such as `thunderbird` works when it is on your path. A full path works too. For the Flatpak build, enter `flatpak run org.mozilla.Thunderbird`.

Emails open in a Thunderbird compose window with the attachments already added. Meetings are passed to Thunderbird as a calendar file; Thunderbird asks which calendar to import into, and you send the invitations from the event.

## Microsoft 365 drafts

This route creates the draft in your mailbox rather than on your machine, so the draft is waiting for you on every device. It needs a Microsoft Entra app registration.

### Register the application once

Your Microsoft 365 administrator creates an app registration that allows these delegated Microsoft Graph permissions:

- `User.Read`
- `Mail.ReadWrite`
- `Calendars.ReadWrite`
- `offline_access`

`Mail.ReadWrite` creates and updates drafts. Project Notes never requests `Mail.Send`, so it cannot send mail from your account even if something goes wrong.

If you also use [File Finder](<FileFinder.md>) to discover Teams channels and their files, the same registration should allow `Team.ReadBasic.All`, `Channel.ReadBasic.All` and `Files.Read.All`.

Some organizations require administrator consent before anyone can sign in.

### Sign in

1. Open **Settings > Office 365 Integration**.
2. Enter the **Microsoft Entra tenant ID**. The default, `organizations`, accepts a work or school account from any organization. Enter a specific tenant ID to restrict it to yours.
3. Enter the **Application (client) ID** from the registration.
4. Click **Sign in to Microsoft 365**. Project Notes shows a short code and a button that opens Microsoft's sign-in page.
5. Click **Copy code**, click **Open Microsoft sign-in page**, paste the code and finish signing in with your browser.
6. Return to Project Notes. The status line turns green when the account is verified.

Your refresh token goes into the operating system credential vault. Access tokens stay in memory only.

7. Go back to **Settings > Preferences** and set **Integration** to **Microsoft 365 drafts**.

If you signed in before meeting scheduling existed, sign out and sign in again. The older sign-in did not ask for calendar permission, and a draft event cannot be created without it.

## Write your email templates

**Settings > Email Templates** wraps your own wording around the report Project Notes generates. Four workflows each have one editable template: **Send Meeting Notes**, **Tracker Items Report**, **Status Report** and **Meeting Notes Report**.

1. Pick the **Workflow**.
2. Edit the **Subject**.
3. Edit the **Rich body**. Keep exactly one `{{ content.body }}` block in it. That block is protected, and it is where the generated report lands. Write your greeting above it and your sign-off below it.
4. **Reset** restores the built-in wording for that workflow.

Right-click the subject or the body to insert a field instead of typing it. The fields that resolve are:

| Field | Value |
| :--- | :--- |
| `project.number` | The project number |
| `project.name` | The project name |
| `client.name` | The client on the project |
| `preferences.managerName` | The project manager from **Settings > Preferences** |
| `preferences.managingCompanyName` | The managing company from **Settings > Preferences** |
| `current.date` | The date the email is prepared, as MM/DD/YYYY |
| `report.type` | The name of the report being sent |
| `report.internal` | `Yes` or `No`, depending on the internal option in the review |
| `report.date` | The reporting date you choose in the review, on the three project reports |
| `meeting.title` | The meeting note's title, on Send Meeting Notes |
| `meeting.date` | The meeting date, on Send Meeting Notes and the Meeting Notes Report |

A field that does not apply to the selected workflow is not offered.

Templates are stored for the open database and the active Project Notes profile. They do not travel with a cloud sync of project data.

## Define your meeting types

**Settings > Meeting Templates** holds the meeting types you can schedule. Nothing is preloaded, so create the ones your organization runs.

1. Click **New**.
2. Give it a **Name**, such as `Customer Status`.
3. Pick a **Duration** between 15 minutes and 4 hours.
4. Write the **Subject**, for example `{{ project.number }} {{ project.name }} Status Meeting`.
5. Write the **Agenda**. It becomes the body of the invitation.

**Duplicate** copies a type when two of them differ only slightly. **Delete** removes one. Leaving the tab saves your changes, and a type left completely blank is discarded.

Subjects and agendas accept `project.number`, `project.name`, `client.name`, `preferences.managerName`, `preferences.managingCompanyName` and `current.date`, which is the date the meeting is scheduled. Right-click to insert one. Report fields such as `report.date` are not offered, because a meeting is scheduled from a project rather than from a report.

Meeting types live in the open database, next to the email templates.

## Test the setup

Run one of each before you rely on it.

1. Open a project that has a team with real email addresses.
2. Right-click it, choose **Reports > Status…**, select **Email the report**, pick one recipient who is you, and click **Send Email**. Your client should open a draft addressed to you.
3. Right-click the project again and choose **Schedule Meeting…**. Pick a meeting type, pick yourself as the only attendee, and click **Open Invite**. Your calendar application should open an event.
4. Send both. Check that they arrive and that the formatting and attachments survived.

## Troubleshooting

| Symptom | Fix |
| :--- | :--- |
| Nothing opens after **Send Email** | No application is registered for `.eml`. Set the association, or switch to **Thunderbird** or **Microsoft 365 drafts**. |
| Nothing opens after **Open Invite** | No application is registered for `.ics`. Same fix. |
| The meeting opens in the new Outlook with no attendees | The new Outlook drops them when it imports a calendar file. Click **Copy Attendees** before **Open Invite** and paste them into the event, or switch to **Outlook (Classic)**. |
| Outlook reports it is not installed | The Outlook you chose is missing. **Outlook** needs the new Outlook for Windows from the Microsoft Store; **Outlook (Classic)** needs Microsoft Office's Outlook. |
| The wrong mail application opens | Another application holds the `.eml` association. Reassign it in your operating system's default applications settings. |
| A person cannot be selected as a recipient | Their person record has no **Email**. Add one. |
| The audience is empty | Nobody is on the project team, or nobody is marked **Receive Status** for a status report. |
| Microsoft 365 reports it is not signed in | Open **Settings > Office 365 Integration** and sign in again. A refresh token expires if it goes unused for a long time. |
| A meeting draft is created but has no attendees | The sign-in predates calendar permission. Sign out, then sign in again to grant `Calendars.ReadWrite`. |
| Calendar reports it is not allowed, or no event appears | Open **System Settings > Privacy & Security**. Under **Calendars**, give Project Notes full access. Under **Automation**, turn on **Calendar** for Project Notes. |
| Mail opens without the attachment | **MacOS Email and Calendar** cannot attach files. Use **Default Application** for that report. |
| Thunderbird does not start | The **Thunderbird executable** value is wrong. Try the bare command `thunderbird`, a full path, or `flatpak run org.mozilla.Thunderbird`. |
| An attachment is missing after you reopen a draft | Staged files are removed when Project Notes closes. Send or save the draft before you quit. |

## Where the temporary files go

Generated messages, calendar files, rendered reports and staged attachments are written to the application cache folder. Project Notes deletes them when it closes, and again at the next start if it did not close cleanly. Sign-in caches are kept.

An open compose window can point at a staged attachment. Send it, or save the attachment somewhere of your own, before you close Project Notes.

## Related Documentation

- [Reports and Email](<ReportsAndEmail.md>) — Generating and sending the four reports
- [Scheduling Meetings](<ScheduleMeeting.md>) — Creating invitations from a project
- [Preferences](<Preferences.md>) — Project manager, managing company and the integration choice
- [Office 365 Integration](<Office365Integration.md>) — Microsoft account settings and credential storage
- [Project Page](<ProjectPage.md>) — Team members, **Receive Status**, and project locations

# Native Reports and Email Review

The QML desktop application has native review paths for these workflows:

- **Send Meeting Notes** for an individual saved meeting note.
- **Meeting Notes Report**, **Status Report**, and **Tracker Items Report** from
  the project's **Generate Report** menu.

The native paths build the report from the saved project data, then show a
review dialog. They do not modify a note, project, or recipient record while
the dialog is open. Save the note or project before starting a report so the
review uses the version you intend to share.

## Review before sharing

The review dialog shows the generated subject and a preview of the message. Where
the chosen mode permits it, you can edit the subject, choose a template, select
the audience shortcut and company filter, adjust recipients, or attach a
file. These review-only changes do not alter the project team or the saved
template unless you explicitly save a template from **Settings > Email
Templates**.

For a generated attachment, Project Notes stages its own copy before a client
handoff. You can use **Save generated attachment** to copy that generated file
to a location you choose. This does not send email or open an email client.

To handle an email yourself, use **Save generated attachment** and send it from
your own client. That action requires a click and leaves the sending to you;
Project Notes does not open or send a message for you.

Choose **Do not email** in a project-report review and then select **Complete
without email** to finalize the reviewed report without requesting an account,
opening a client, or calling an email backend. The saved project data is
revalidated first, so regenerate the review if Project Notes reports that the
source changed.

For the three project reports, **Generate HTML report** has a separate meaning
from **HTML attachment**: it keeps a standalone HTML copy in the project's
configured report folder, while HTML attachment only stages a copy for the
chosen email handoff. Project Notes uses the report's configured subfolder
under the saved local project folder. If that folder is missing, unwritable, or
unsafe, the review keeps its staged file and shows a destination warning rather
than writing somewhere else or changing the filename.

Set that subfolder in **Settings > Email Templates > Report folders**. Select
the report workflow, enter a relative path such as
`Project Management/Status Reports`, and choose whether it applies to the
current database only or to the active profile generally. A database-specific
value takes precedence over the profile value. The native defaults are
`Project Management/Issues List` for Tracker Items, `Project Management/Status
Reports` for Status, and `Project Management/Meeting Minutes` for Meeting
Notes. Paths outside the saved project folder, including absolute paths and
`..` traversal, are rejected.

**Display report when complete** independently requests a staged PDF for local
presentation. It does not turn an inline report into a PDF attachment and does
not require an email account. If the desktop cannot open the file, the staged
PDF remains available for recovery and the review shows a warning.

## Templates and recipients

Use **Settings > Email Templates** to choose a workflow and create a custom
wrapper around the native report content. The generated content block is kept
protected so a template cannot accidentally omit or duplicate the report.
Templates are scoped to the active Project Notes profile and database.

Right-click a subject or body to insert a field. **current.date** inserts the
date the email is prepared, written as MM/DD/YYYY.

A template may retain a reference to one global saved audience preset for its
workflow. It is only a reusable suggestion: saving or choosing that reference
never changes recipients. Apply the preset from a review, where the
current project and immutable recipient snapshot are known.
If a referenced preset is later removed, Settings shows that missing reference
until you explicitly select a replacement or clear it.

Audience controls resolve recipients from the current saved snapshot. Common
shortcuts include the project team, meeting attendees, status-report
recipients, the captured current selection, and an explicit **Choose people**
list. You can refine an audience by managing company, project client, everyone
except the client, or selected companies. The grouped checklist shows why a
person is included, unique To/Cc/Bcc totals, and a collapsible list of contacts
not included with the resolver's reason. Company filters, manager exclusion,
and recipient-role changes affect only the open review. An
internal report warns if its selected audience includes someone outside the
configured managing company. Check the displayed list before creating a draft,
especially after changing an audience source.

Recipients always come from people records. To email someone who isn't listed,
add them as a person and put them on the project team first. You can't type an
address into the review.

Every Generate Report review shows the saved-audience controls. There is no
separate Advanced section. Pick a saved audience from the dropdown and the
checklist updates immediately. **Save audience** stores the current selection
for the project. Saved audiences are shared, so one you save while running a
Status Report also appears for the Tracker Items Report and the Meeting Notes
Report on the same project.

Defaults work per report. **Set as report default** makes the chosen audience
load automatically the next time you generate the report you are reviewing for
this project. Each report keeps its own default, and a single audience can be
the default for several reports. The dropdown marks the current report's
default. Without a default, Status Report and Tracker Items Report start with
the team members marked to receive status reports.

## Meeting templates

**Settings > Meeting Templates** holds your meeting types. Email templates cover
a fixed set of reports. Meeting types are yours to define: add as many as you
need with **New**, copy one with **Duplicate**, or remove one with **Delete**.

Each meeting type has a name, a default duration, an invitation subject and a
formatted agenda. Calendar files also carry a plain-text description, which
Project Notes derives from that agenda.

Subjects and agendas can insert project number, project name, client name,
the preference fields for project manager and managing company, and
**current.date**, the date the meeting is scheduled, written as MM/DD/YYYY. Right-click a
subject or agenda to insert one. Meeting-note and report fields such as
`meeting.date` are not available, because a meeting is scheduled from a project
rather than from one note or report.

Meeting types are stored in the open database, next to the email templates.
Leaving the tab saves your changes. A new meeting type left completely blank is
discarded. Meeting types defined in the legacy **Meeting and Email Types**
plugin setting are not imported.

## Scheduling a meeting

Right-click a project, or open the project's own menu, and choose **Schedule
Meeting…**. Pick a meeting type from **Settings > Meeting Templates**. Project
Notes fills in its subject and agenda from the project, proposes the next whole
hour as the start time and uses the type's duration. Pick the attendees, then
click **Open Invite**. The invite opens straight away in your calendar app, with
no confirmation box in between. Change the date, time, duration, location,
subject or agenda there.

Attendees come from the project team, just as report recipients do. Each person
is **Required** or **Optional**. The saved audiences are the same ones the
reports use: an audience saved here appears in the Generate Report review, and
the reverse. Required is stored as To and Optional as Cc, so a report's Bcc
recipients load as Optional. **Set as meeting default** loads that audience
automatically the next time you schedule a meeting for the project.

The organizer is the project manager chosen in **Settings > Preferences**, when
that person has an email address. The organizer is never also listed as an
attendee.

What happens next depends on your Email and Scheduling Integration:

- **Default Application** writes a calendar file (.ics) and opens it in your
  default calendar app. Save the event there and send the invitations.
- **Thunderbird** passes the calendar file to Thunderbird, which offers to
  import it into one of its calendars. Open the event to send the invitations.
- **Microsoft 365 drafts** creates a draft event in your Microsoft 365 calendar
  and opens it in Outlook on the web, where you review it and send the
  invitations. This needs calendar permission, so sign out and sign in again
  under **Office 365 Integration** if you signed in before this feature existed.

Microsoft does not document that a draft event with attendees is held back from
them. Try the Microsoft 365 route with a test meeting before relying on it.

## Email and Scheduling Integration

**Settings > Preferences > Email and Scheduling Integration** chooses one route
for both emails and meeting invitations: Default Application, Thunderbird, or
Microsoft 365 drafts. For Thunderbird, enter its executable path or a command
such as `flatpak run org.mozilla.Thunderbird`.

**Default Application** writes the email as a draft message file (.eml) in the
application cache and opens it with whatever mail application your desktop uses
for that file type. The draft carries the formatted report and its attachments,
so there is nothing to attach by hand. Outlook opens such a file as a new,
unsent message; some clients show it as a message you then edit or forward.
Meeting invitations take the matching route: a calendar file (.ics) opened in
your default calendar app.

A preference is not proof that the client is installed, signed in, or
available. Client focus after a handoff is only an attempt; it is never a
delivery confirmation. Project Notes never sends an email or an invitation
itself. You send it from the client.

Microsoft 365 creates drafts only after you sign in under **Office 365
Integration** and Project Notes verifies the account. Signing in requests
permission to create email drafts and draft calendar events. It never requests
permission to send mail.

## Temporary files

Staged attachments, rendered reports, calendar files and the report files the
export plugins write are kept in the application cache folder. Project Notes
removes them when it closes, and again at the next start if it did not close
normally. Plugin sign-in caches and saved plugin state in the same folder are
kept. Send or save anything you still need from an open compose window before
you close Project Notes, because an attachment it points to may be gone.

## Legacy plugin help

The older **Export Meeting Notes**, **Export Status Report**, **Export Tracker
Items**, and **Send Meeting Notes** plugin pages remain in this guide while the
native replacement is being certified. They are preserved for existing
installations and custom workflows. Project Notes will not remove those bundled
plugins, user plugins, or user settings until a future upgrade has passed the
explicit removal gate.

The legacy **Schedule Meeting** plugin entries remain alongside the native
**Schedule Meeting…** action until their meeting types have been moved to
**Settings > Meeting Templates**.

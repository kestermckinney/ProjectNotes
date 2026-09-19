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

The review dialog shows the generated subject and a plain-text summary. Where
the chosen mode permits it, you can edit the subject, choose a template, select
the audience shortcut and company filter, adjust recipients, or attach a
file. These review-only changes do not alter the project team or the saved
template unless you explicitly save a template from **Settings > Email
Templates**.

For a generated attachment, Project Notes stages its own copy before a client
handoff. You can use **Save generated attachment** to copy that generated file
to a location you choose. This does not send email or open an email client.

For a manual email, use **Copy plain-text message** and, when present, **Save
generated attachment**. Both actions require a click and leave it to you to
paste the message and attach the saved file in your chosen client; Project
Notes does not open or send a manual message.

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

## Email-client availability

The Email Templates page records a preferred backend: the default plain-text
client, Thunderbird, or a Microsoft 365 draft. A preference is not proof that
the corresponding client is installed, signed in, or available. Client focus
after a handoff is only an attempt; it is never a delivery confirmation.

The packaged Qt WebEngine runtime and local mail-client dispatch are still
being certified on supported platform packages. The local-client routes remain
unavailable until that work is complete. The review and save-generated-file
actions remain safe to use without an email account.

Microsoft 365 is the currently available draft route. **Create Microsoft 365
draft** appears only after you explicitly enable its separate mail-draft
capability and Project Notes verifies the signed-in account identity. The
action creates a draft only; it never sends email or creates a calendar item.
After a successful draft, **Open draft in Outlook** is an optional user click.
The existing Office 365 File Finder sign-in remains separate and continues to
use only its file-discovery capabilities.

## Legacy plugin help

The older **Export Meeting Notes**, **Export Status Report**, **Export Tracker
Items**, and **Send Meeting Notes** plugin pages remain in this guide while the
native replacement is being certified. They are preserved for existing
installations and custom workflows. Project Notes will not remove those bundled
plugins, user plugins, or user settings until a future upgrade has passed the
explicit removal gate.

Scheduling/calendar plugins are outside the native email work. Project Notes
does not create or update calendar invitations through the native review path.

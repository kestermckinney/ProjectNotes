# Scheduling Meetings

Project Notes turns a project and a meeting type into a ready invitation, then opens it in your calendar application. You confirm the details and send the invitations from there.

Two things must exist first: at least one meeting type in **Settings > Meeting Templates**, and an integration chosen in **Settings > Preferences**. Project Notes adds standard meeting types at startup when the list is empty. Both settings are covered in [Email and Meeting Setup](<EmailAndMeetingSetup.md>).

## Creating an invitation

1. Right-click a project, on the Projects list or on the project's own page, and choose **Schedule Meeting…**.
2. Under **Which meeting?**, pick a **Meeting type**. Project Notes fills the subject and agenda from the project and the type.
3. Under **Who should attend?**, check the contacts to invite and set each one **Required** or **Optional**.
4. Click **Open Invite**.

The invite opens straight away. There is no confirmation step between the button and your calendar application.

If the meeting type list is empty, the dialog says so. Add a type in **Settings > Meeting Templates** and try again.

## What Project Notes fills in

| Field | Value |
| :--- | :--- |
| Subject | The meeting type's subject, with its project fields resolved |
| Body | The meeting type's agenda |
| Start | The next whole hour |
| Duration | The meeting type's duration |
| Organizer | The project manager from **Settings > Preferences**, when that contact has an email address |
| Attendees | The contacts you checked, as Required or Optional |

The organizer is never also listed as an attendee. Everything above is editable in your calendar application, so treat the proposed time as a starting point rather than a decision.

There is no location field in the dialog. Add the room or the meeting link in your calendar application before you send.

## Choosing attendees

Attendees come from the project team, exactly as report recipients do. A contact with no email address cannot be selected, and you cannot type an address into the dialog.

Saved audiences are shared with the project's reports. An audience saved here appears in the **Run report** dialog, and one saved there appears here. **Required** is stored as To and **Optional** as Cc, so a report's Bcc recipients load as Optional.

**Set as meeting default** loads that audience automatically the next time you schedule a meeting for this project.

## What happens after Open Invite

The route depends on **Settings > Preferences > Email, Contact and Calendar Integration**. The list there only offers the choices your platform supports; see [Email and Meeting Setup](<EmailAndMeetingSetup.md>) for which appear where.

Some routes cannot carry the attendees. For those, the Schedule meeting window shows a **Copy Attendees** button that puts every required and optional attendee on the clipboard, ready to paste into the event.

**Default Application** (all platforms) writes a calendar file and opens it with whatever application handles `.ics` files. Many calendar applications, the new Outlook among them, import the file without its attendees, so click **Copy Attendees** first and paste them into the event. Then confirm the time and send the invitations.

**Outlook** (Windows) opens the calendar file in the new Outlook, whatever application holds the `.ics` association. The new Outlook drops the attendees, so click **Copy Attendees**, paste them into the event, then send it.

**Outlook (Classic)** (Windows) builds the meeting in Outlook (classic) and opens it as an unsent meeting window. The attendees, time and agenda are already filled in: required attendees on the **To** line, optional ones on the optional line. Check it, then click **Send**.

**MacOS Email and Calendar** (macOS) creates the event directly in Calendar, on your first writable calendar, with the agenda in its notes. Calendar would send invitations the moment an event gets attendees, so none are added. Click **Copy Attendees** in the Schedule meeting window before or after **Open Invite**. That puts every required and optional attendee on the clipboard. Paste them into the event's invitees field in Calendar, then click **Send**. The first time, macOS asks to let Project Notes access your calendars and control Calendar. Allow both.

**iCloud** (all platforms) creates the event in iCloud Calendar without attendees. Click **Copy Attendees** before **Create Draft**, then open iCloud Calendar, find the new event, paste the attendees and send the invitations. iCloud does not provide a link to the newly created event.

**Thunderbird** (all platforms) hands Thunderbird the calendar file. Thunderbird asks which of its calendars to import it into. Open the event there to edit it and send the invitations.

**Microsoft 365 drafts** (all platforms) creates a draft event in your Microsoft 365 calendar and opens it in Outlook on the web. Review it and send the invitations from there. This route needs calendar permission, so sign out and sign in again under **Settings > Office 365 Integration** if your sign-in predates meeting scheduling.

Project Notes never sends an invitation. It only prepares one.

## Related Documentation

- [Email and Meeting Setup](<EmailAndMeetingSetup.md>) — Meeting types, integration choice and platform setup
- [Reports and Email](<ReportsAndEmail.md>) — Reports, recipients and the shared saved audiences
- [Preferences](<Preferences.md>) — Project manager, managing company and the integration choice
- [Project Page](<ProjectPage.md>) — The project team that attendees are drawn from

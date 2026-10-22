# Project Notes Mobile

Project Notes Mobile is an iOS companion app for Project Notes. It gives project managers access to their projects, notes, tracker items, contacts, and companies on their iPhone or iPad. Data is synchronized with a cloud backend so the mobile app stays in sync with the desktop application.

## Navigation

The app uses a bottom tab bar with four top-level tabs and a side drawer (hamburger menu) for settings and utilities.

**Bottom Tabs:**

| Tab | Description |
| :--- | :--- |
| Projects | List of all projects |
| Contacts | List of all team members |
| Companies | List of all companies |
| Items | Unified list of all tracker items across all projects |

**Hamburger Menu (top-left ≡ button):**

| Item | Description |
| :--- | :--- |
| Cloud Sync Settings | Open the [Sync Settings](#sync-settings) page |
| View | Open the [View Options](#view-options) page |
| Sync All… | Manually trigger a full sync with the cloud server |
| Preferences… | Open the [Preferences](#preferences) page |
| Help | Open this documentation |
| What's New | Open the release notes |
| About | Display the About screen with version information |

---

## Pages

### Projects List Page

The entry point for project work. Displays all projects as a scrollable list. A **Quick Search** bar at the top filters the list in real time as you type.

- Tap a project to open the [Project Details Page](#project-details-page).
- The list respects the **Show Closed Projects** setting in [View Options](#view-options).
- Tap the filter button and choose **Folder** to show only the projects in one or more project folders. Every folder is listed, along with **Not Categorized** for projects in no folder. Check as many as you like and tap **Apply**. Folders are created and named in the desktop app and sync to the phone with the rest of your data.

### Project Details Page

Shows the full details for a single project, organized into tabs:

| Tab | Contents |
| :--- | :--- |
| Info | Project name, number, status, managing company, project manager, and description |
| Notes | All meeting notes for the project — tap a note to open the [Note Detail Page](#project-note-detail-page) |
| Tracker | Tracker and action items for the project — tap an item to open the [Tracker Item Detail Page](#tracker-item-detail-page) |
| Locations | Associated file paths and locations — tap an entry to open the [Location Detail Page](#project-location-detail-page) |
| Team | Team members assigned to the project — tap a member to open the [Team Member Detail Page](#team-member-detail-page) |

### Project Notes Page

Lists all meeting notes associated with a project. A **Quick Search** bar at the top filters notes by title or content as you type.

- Tap a note to open the [Project Note Detail Page](#project-note-detail-page).
- Notes are listed in reverse chronological order.

### Project Note Detail Page

Displays the full content of a single meeting note.

| Field | Description |
| :--- | :--- |
| Title | Note title or meeting subject |
| Date | Date of the meeting or note |
| Internal | Marks the note as internal (not for company distribution) |
| Content | Full rich-text body of the note |
| Attendees | List of meeting attendees — tap to open the [Meeting Attendees Page](#meeting-attendees-page) |
| Action Items | Action items generated from this note — tap to open the [Note Action Items Page](#note-action-items-page) |

**Emailing meeting notes:** When you email a meeting note, Project Notes Mobile opens the native iOS mail composer (the standard Mail compose sheet) pre-populated with the attendees as recipients and a message body that includes the note content, its action items, and the attendee list. You can review and edit the message before sending, and it is sent through your device's configured mail account. (Earlier builds opened a plain `mailto:` link, which could not include the full action-item and attendee details.)

### Meeting Attendees Page

Lists all attendees for a specific meeting note. Tap an attendee to open the [Meeting Attendee Detail Page](#meeting-attendee-detail-page).
Touch and hold an attendee to share their [contact card](#sharing-a-contact-card).

### Meeting Attendee Detail Page

Displays the name and contact details for a single meeting attendee.

### Note Action Items Page

Lists the action items associated with a specific meeting note. Tap an item to open the [Tracker Item Detail Page](#tracker-item-detail-page).

### Project Tracker Page

Lists all tracker items (risks, issues, action items, work items, change requests) for a project. The list respects the **Show Internal Items** and **Show Resolved Items** settings in [View Options](#view-options).

### Tracker Item Detail Page

Displays the full details for a single tracker item.

| Field | Description |
| :--- | :--- |
| Type | Item type: Risk, Issue, Action Item, Work Item, or Change Request |
| Item Number | Unique item number within the project |
| Title | Short description of the item |
| Status | Current status: New, Assigned, In Progress, Resolved, Closed |
| Priority | Priority level: Low, Medium, High, Critical |
| Assigned To | Team member responsible for the item |
| Meeting | The meeting note this item originated from, chosen from a drop-down of the project's notes. Linking an item to its meeting ties the item back to where it was raised. |
| Due Date | Target completion date |
| Opened Date | Date the item was opened |
| Closed Date | Date the item was resolved or closed |
| Internal | Marks the item as internal |
| Description | Full description of the item |
| Comments | Threaded comments — tap to open the [Tracker Item Comments Page](#tracker-item-comments-page) |

### Tracker Item Comments Page

Lists all comments attached to a tracker item in chronological order. Tap a comment to open the [Tracker Item Comment Detail Page](#tracker-item-comment-detail-page).

### Tracker Item Comment Detail Page

Displays and allows editing of a single comment on a tracker item.

### Status Items Page

Lists status report items for a project. These are high-level summary items used for status reports and company presentations.

### Status Item Detail Page

Displays the details for a single status item.

| Field | Description |
| :--- | :--- |
| Type | Status item category |
| Description | Item description |
| Internal | Marks the item as internal |

### Project Location Detail Page

Displays an associated file path or folder location for a project.

| Field | Description |
| :--- | :--- |
| Description | Human-readable label for the location |
| Path | File system path or URL |

When the path is a web link, an **Open** button appears. For a Word, Excel, PowerPoint, or Project document stored in SharePoint/Teams, it first tries to launch the matching Office app directly to that document, falling back to Safari when the app isn't installed. A local file system path (e.g. from a desktop File Finder scan) can't be opened from the mobile app.

### Team Members Page

Lists all team members assigned to a specific project.

Touch and hold a team member to share their [contact card](#sharing-a-contact-card).

### Team Member Detail Page

Displays the relationship between a team member and a project, including their role on that project.

### All Items Page

A unified view of tracker items across **all** projects. Useful for seeing all open or assigned items regardless of which project they belong to. The list respects the same View Option filters as the per-project tracker page.

- Tap an item to open the [Tracker Item Detail Page](#tracker-item-detail-page).

### Contacts Page

Lists all team members in the database. A **Quick Search** bar filters the list as you type.

- Tap a contact to open the [Contact Detail Page](#contact-detail-page).
- Touch and hold a contact to share their [contact card](#sharing-a-contact-card). When the contact has a company, the same menu offers **Quick Filter…** to show only that company's contacts.

### Sharing a Contact Card

Touch and hold a contact, team member, or meeting attendee to open its contact card menu.

- **Email Contact…** opens the iOS Mail compose sheet with the person's vCard (`.vcf`) attached and the subject `Contact: <name>`. Add the recipients before you send. If Mail has no account set up, the share sheet opens instead so you can pick another mail app.
- **Export Contact…** opens the iOS share sheet. Save the card to Files, AirDrop it, send it in Messages, or open it in another app.

The card holds the name, company, role, email, office phone and cell phone.

### Contact Detail Page

Displays the full contact record for a team member.

| Field | Description |
| :--- | :--- |
| Name | Full name |
| Email | Email address |
| Office Phone | Office phone number |
| Cell Phone | Mobile phone number |
| Role | Job title or role |
| Company | Associated company |

A phone-icon button appears next to **Office Phone** and **Cell Phone** when a number is present. Tapping the button opens your phone's dialer with that number pre-filled, using a `tel:` URL. The same call buttons appear on the Team Member Detail Page and Meeting Attendee Detail Page wherever a phone number is shown.

### Companies Page

Lists all companies in the database. A **Quick Search** bar filters the list as you type.

- Tap a company to open the [Company Detail Page](#company-detail-page).

### Company Detail Page

Displays the details for a company.

| Field | Description |
| :--- | :--- |
| Name | Company name |
| Address | Mailing address |
| Phone | Main phone number |
| Website | Company website |
| Notes | Freeform notes about the company |

---

## Editing and Deleting Records

Detail pages show the project and note context in their title bar so you always know which record you are editing.

When you delete a record that other records depend on — for example, a contact who is still assigned to projects or a note that still has action items — the database blocks the delete to protect your data. The app now reports this with a clear message and keeps the record open, instead of appearing to do nothing. Remove or reassign the dependent records first, then try the delete again.

---

## Sync Settings

Open from **≡ → Cloud Sync Settings**.

The mobile app synchronizes data with **Project Notes Pro** hosting using [SqliteSyncPro](https://gitlab.com/kestermckinney/SqliteSyncPro). Hosting is fully managed — there is no server address or API key to enter. You sign in with the same Project Notes Pro account you use on the desktop, and your data stays in sync across every device.

| Setting | Description |
| :--- | :--- |
| Enable Sync | Master on/off toggle for cloud synchronization. When off, the app operates in local-only mode and no network requests are made. |
| Email | The email address for your Project Notes Pro account. |
| Password | The password for your Project Notes Pro account. |
| Encryption Phrase | An optional passphrase used to encrypt your data before it is sent to the server. If set, the same phrase must be configured on every device that syncs this account. Leave blank to disable encryption. |

The page also shows your current **subscription status** and a link to [www.projectnotespro.com](https://www.projectnotespro.com), where you manage your subscription, along with the Project ID your data is associated with.

**Checking your settings:**

Each field saves as soon as you leave it, so a mistyped password or a mismatched encryption phrase would otherwise sit unnoticed until you wondered why nothing was syncing. When you change one of these fields and then tap **Back**, the app signs in to Project Notes Pro with what you entered and confirms your encryption phrase opens the records already stored for your account. If something is wrong, a dialog appears before the page closes:

- **Credentials rejected** — the email and password were not accepted. Sync will not run until they are corrected.
- **Encryption phrase mismatch** — sign-in worked, but none of your stored records can be decrypted with the phrase you entered, so records from your other devices would be skipped. Every device on the account must use the same phrase.
- **Host unreachable** — your settings were saved but could not be checked. They will be checked again the next time sync runs.

Tap **Back to Settings** to stay and fix the field, or **Leave Anyway** to continue — your entries stay saved either way. The desktop app performs the same check when you navigate away from its Settings screen.

**Sync behavior:**

- The app syncs automatically in the background on a 30-second interval when **Enable Sync** is on.
- Use **≡ → Sync All…** to trigger an immediate full sync at any time.
- A thin progress bar below the navigation bar indicates sync activity: green while syncing, red if an error occurs.
- Sync errors are written to `error.log` in the app's data container.

---

## View Options

Open from **≡ → View**.

View options control which records are shown in list pages throughout the app. Changes take effect immediately without requiring a restart or manual refresh.

| Option | Default | Description |
| :--- | :---: | :--- |
| Show Closed Projects | On | When enabled, projects with a **Closed** status appear in the Projects list. Turn this off to hide completed projects and reduce clutter. |
| Show Internal Items | Off | When enabled, tracker items and notes that are marked **Internal** are visible. Turn this on to see items that are not intended for company distribution. |
| Show Resolved Items | Off | When off, the tracker and All Items pages show only items with a status of **New** or **Assigned** — **Resolved**, **Deferred** and **Canceled** items stay hidden, which keeps the focus on open work. Turn it on to see every item. Matches **Show Resolved Items** in the desktop app's View menu. |

---

## Preferences

Open from **≡ → Preferences…**.

Preferences configure default values that are applied when creating new records.

| Setting | Description |
| :--- | :--- |
| Managing Company | The company that is pre-selected as the managing company on new projects. Choose your own organization from the Companies list. |
| Project Manager | The team member that is pre-selected as the project manager on new projects. Choose yourself from the Contacts list. |

Setting these values saves time when creating projects because you will not need to fill in the same managing company and project manager on every new project.

---

## About Page

The About page displays the application name, version number, and copyright information. Open from **≡ → About**.

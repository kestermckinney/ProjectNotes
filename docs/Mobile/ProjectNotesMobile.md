# Project Notes Mobile

Project Notes Mobile is an iOS companion app for Project Notes. It gives project managers access to their projects, notes, tracker items, contacts, and clients on their iPhone or iPad. Data is synchronized with a cloud backend so the mobile app stays in sync with the desktop application.

## Navigation

The app uses a bottom tab bar with four top-level tabs and a side drawer (hamburger menu) for settings and utilities.

**Bottom Tabs:**

| Tab | Description |
| :--- | :--- |
| Projects | List of all projects |
| People | List of all team members |
| Clients | List of all client companies |
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
| Internal | Marks the note as internal (not for client distribution) |
| Content | Full rich-text body of the note |
| Attendees | List of meeting attendees — tap to open the [Meeting Attendees Page](#meeting-attendees-page) |
| Action Items | Action items generated from this note — tap to open the [Note Action Items Page](#note-action-items-page) |

### Meeting Attendees Page

Lists all attendees for a specific meeting note. Tap an attendee to open the [Meeting Attendee Detail Page](#meeting-attendee-detail-page).

### Meeting Attendee Detail Page

Displays the name and contact details for a single meeting attendee.

### Note Action Items Page

Lists the action items associated with a specific meeting note. Tap an item to open the [Tracker Item Detail Page](#tracker-item-detail-page).

### Project Tracker Page

Lists all tracker items (risks, issues, action items, work items, change requests) for a project. The list respects the **Show Internal Items** and **New and Assigned Only** settings in [View Options](#view-options).

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

Lists status report items for a project. These are high-level summary items used for status reports and client presentations.

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

### Team Members Page

Lists all team members assigned to a specific project.

### Team Member Detail Page

Displays the relationship between a team member and a project, including their role on that project.

### All Items Page

A unified view of tracker items across **all** projects. Useful for seeing all open or assigned items regardless of which project they belong to. The list respects the same View Option filters as the per-project tracker page.

- Tap an item to open the [Tracker Item Detail Page](#tracker-item-detail-page).

### People Page

Lists all team members in the database. A **Quick Search** bar filters the list as you type.

- Tap a person to open the [Person Detail Page](#person-detail-page).

### Person Detail Page

Displays the full contact record for a team member.

| Field | Description |
| :--- | :--- |
| Name | Full name |
| Email | Email address |
| Office Phone | Office phone number |
| Cell Phone | Mobile phone number |
| Role | Job title or role |
| Company | Associated client company |

### Clients Page

Lists all client companies in the database. A **Quick Search** bar filters the list as you type.

- Tap a client to open the [Client Detail Page](#client-detail-page).

### Client Detail Page

Displays the details for a client company.

| Field | Description |
| :--- | :--- |
| Name | Company name |
| Address | Mailing address |
| Phone | Main phone number |
| Website | Company website |
| Notes | Freeform notes about the client |

---

## Sync Settings

Open from **≡ → Cloud Sync Settings**.

The mobile app synchronizes data with a cloud backend using [SqliteSyncPro](https://github.com/kestermckinney/SqliteSyncPro). Two backend types are supported: a self-hosted [PostgREST](https://postgrest.org) server or a [Supabase](https://supabase.com) project.

| Setting | Description |
| :--- | :--- |
| Enable Sync | Master on/off toggle for cloud synchronization. When off, the app operates in local-only mode and no network requests are made. |
| Host Type | Selects the backend type: **Self-Hosted PostgREST** or **Supabase**. This choice controls which additional fields are shown below. |
| Server URL | The base URL of your PostgREST REST API endpoint (e.g., `https://your-server/rest/v1`). Shown when **Self-Hosted PostgREST** is selected. |
| Supabase URL | The URL of your Supabase project (e.g., `https://xxxx.supabase.co`). Shown when **Supabase** is selected. |
| Supabase API Key | The `anon` public key for your Supabase project. Shown when **Supabase** is selected. |
| Email | The email address used to authenticate with the sync server. |
| Password | The password for the sync account. |
| Encryption Phrase | An optional passphrase used to encrypt sensitive data before it is sent to the server. If set, the same phrase must be configured on every device that syncs to this database. Leave blank to disable encryption. |

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
| Show Internal Items | Off | When enabled, tracker items and notes that are marked **Internal** are visible. Turn this on to see items that are not intended for client distribution. |
| New and Assigned Only | Off | When enabled, the tracker and All Items pages show only items with a status of **New** or **Assigned**. This is useful for focusing on open work items. |

---

## Preferences

Open from **≡ → Preferences…**.

Preferences configure default values that are applied when creating new records.

| Setting | Description |
| :--- | :--- |
| Managing Company | The client company that is pre-selected as the managing company on new projects. Choose your own organization from the Clients list. |
| Project Manager | The team member that is pre-selected as the project manager on new projects. Choose yourself from the People list. |

Setting these values saves time when creating projects because you will not need to fill in the same managing company and project manager on every new project.

---

## About Page

The About page displays the application name, version number, and copyright information. Open from **≡ → About**.

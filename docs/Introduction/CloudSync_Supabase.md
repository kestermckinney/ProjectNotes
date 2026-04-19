# Setting Up Cloud Sync with Supabase

Supabase is the easiest way to get cloud sync running. It provides a managed PostgreSQL database with PostgREST built in, a web dashboard, and a generous free tier. No server administration is required.

The **Project Notes Remote Host** admin tool handles all database schema setup and user management. You do not need to run any SQL manually.

When you are done with setup, see [Remote Host](<../InterfaceOverview/RemoteHost.md>) for a full reference of all sync features and settings.

## Prerequisites

- A Supabase account ([supabase.com](https://supabase.com))
- The **Project Notes Remote Host** admin tool installed
- Project Notes installed and running

## Step 1: Create a Supabase Project

1. Sign in to your Supabase account at [supabase.com](https://supabase.com).
2. Click **New project**.
3. Choose or create an **Organization**.
4. Enter a **Project Name** (e.g., `project-notes`).
5. Set a strong **Database Password** and save it somewhere secure.
6. Choose the **Region** closest to your primary work location.
7. Click **Create new project**.

Supabase will provision your database. This typically takes one to two minutes.

## Step 2: Retrieve Your Admin Credentials

Once the project is ready:

1. In the left sidebar, click the **gear icon** (Project Settings).
2. Click **API** under the Settings menu.
3. Note the following values — you will need them in the admin tool:

| Value | Where to Find It |
| :--- | :--- |
| **Project URL** | Listed as "Project URL" near the top of the API settings page (e.g., `https://abcdefgh.supabase.co`) |
| **Service Role Key** | Listed under "Project API Keys" as `service_role` — used by the admin tool to configure the database |
| **Anon / Public Key** | Listed under "Project API Keys" as `anon public` — entered into Project Notes |

> **Security note:** The `service_role` key has full database access. Only enter it in the admin tool during setup — do not share it or enter it into Project Notes.

## Step 3: Run the Admin Tool

The **Project Notes Remote Host** admin tool initializes the database schema and creates user accounts. Run it once from any machine.

1. Open the **Project Notes Remote Host** application.
2. On the **Connection** page:
   - Select **Supabase** as the hosting mode.
   - Enter your Supabase **Project URL**.
   - Enter your **Service Role Key**.
   - Click **Test Connection** to verify, then click **Next**.
3. On the **Configure** page, review the connection details and click **Next**.
4. On the **Install** page, the tool runs through 21 setup steps — creating roles, the sync table, row-level security policies, and Supabase Auth integration. Watch the progress log and wait for setup to complete, then click **Next**.
5. On the **Users** page, add each person who will sync Project Notes data:
   - Click **Add User**, enter a username (email address) and password, and click **Save**.
   - Repeat for each user.
   - Click **Next** when done.
6. On the **Summary** page, copy and save the displayed configuration values. Click **Next**.
7. The wizard is complete. You can close the admin tool.

## Step 4: Configure Cloud Sync in Project Notes

Each user configures Project Notes with their own credentials:

1. Open Project Notes.
2. From the **File** menu, choose **Cloud Sync Settings...**
3. Check **Sync data with cloud host**.
4. Set **Sync Host Type** to **Supabase**.
5. Enter the **Server URL** — your Supabase Project URL (e.g., `https://abcdefgh.supabase.co`).
6. Enter your **Username (Email)** and **Password** — the credentials created for this user in the admin tool.
7. Enter the **Supabase Anon Key** — the `anon public` key from Step 2.
8. Optionally enter an **Encryption Phrase** to encrypt data at rest on the server. Store this phrase safely — it cannot be recovered, and every machine syncing to this project must use the same phrase.
9. Click **OK**.

Project Notes will verify the connection. If it succeeds, sync will begin automatically.

## Step 5: Verify Sync Is Working

1. After saving settings, observe the sync progress bar in the bottom-right corner of the status bar.
2. The bar fills as records are pushed to Supabase and disappears when sync is complete.
3. To force a full sync, choose **File > Sync All**.

If the connection fails, a warning dialog will appear. Double-check your Project URL, Anon Key, and credentials, then retry.

## Managing Users

To add, remove, or change passwords for users after initial setup:

1. Open the **Project Notes Remote Host** admin tool.
2. On the **Connection** page, enter your Supabase credentials and check **Already set up**, then click **Next** twice to skip to the **Users** page.
3. Add, edit, or remove users as needed.

## Notes and Limitations

- The Supabase free tier has limits on database size and API requests. For teams with large databases or high sync frequency, consider upgrading to a paid Supabase plan.
- The **Encryption Phrase** encrypts data before it is sent to Supabase. All machines syncing to the same project must use the same phrase.
- Supabase Project URLs and anon keys are stable — they do not change unless you delete and recreate the project.

## Related Pages

- [Remote Host](<../InterfaceOverview/RemoteHost.md>) — Full feature reference for all Cloud Sync settings and behaviors
- [Setting Up Neon](<CloudSync_Neon.md>)
- [Setting Up a Self-Hosted PostgREST Server](<CloudSync_SelfHosted.md>)
- [File Menu](<../InterfaceOverview/FileMenu.md>) — Cloud Sync Settings dialog reference

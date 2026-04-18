# Setting Up Cloud Sync with Neon

Neon is a serverless PostgreSQL provider that includes a built-in **Data API** — a PostgREST-compatible REST interface — so you do not need to run a separate PostgREST server. Project Notes connects directly to the Neon Data API endpoint.

The **Project Notes Remote Host** admin tool handles all database schema setup and user management. You do not need to run any SQL manually.

When you are done with setup, see [Remote Host](<../InterfaceOverview/RemoteHost.md>) for a full reference of all sync features and settings.

## Prerequisites

- A Neon account ([neon.tech](https://neon.tech))
- The **Project Notes Remote Host** admin tool installed
- Project Notes installed and running

## Step 1: Create a Neon Project and Database

1. Sign in to your Neon account at [neon.tech](https://neon.tech).
2. Click **New Project**.
3. Enter a **Project Name** (e.g., `project-notes`).
4. Choose a **Region** closest to your users.
5. Click **Create Project**.

Neon creates a PostgreSQL database and displays a **Connection String**. Copy it — you will use it in the admin tool. It looks like:

```
postgresql://username:password@ep-xxxx.us-east-2.aws.neon.tech/neondb?sslmode=require
```

## Step 2: Enable the Neon Data API

Neon's Data API provides a PostgREST-compatible REST endpoint for your database.

1. In the Neon dashboard, navigate to your project.
2. Click **Settings** in the left sidebar.
3. Select **Data API**.
4. Enable the Data API and note the **Data API URL** — this is the endpoint you will enter into Project Notes as the Server URL (e.g., `https://ep-xxxx.us-east-2.aws.neon.tech/v1`).

## Step 3: Run the Admin Tool

The **Project Notes Remote Host** admin tool initializes the database schema, creates the required roles (including `pnanon`), and sets up user accounts. Run it once from any machine.

1. Open the **Project Notes Remote Host** application.
2. On the **Connection** page:
   - Select **Self-Hosted** as the hosting mode.
   - Enter your Neon PostgreSQL connection details (host, port, database name, superuser, and password from the connection string in Step 1).
   - Click **Test Connection** to verify, then click **Next**.
3. On the **Configure** page, review the connection details and click **Next**.
4. On the **Install** page, the tool runs through 21 setup steps — creating the `pnanon` role, `pnapp_user` role, sync table, row-level security policies, and login functions. Watch the progress log and wait for setup to complete, then click **Next**.
5. On the **Users** page, add each person who will sync Project Notes data:
   - Click **Add User**, enter a username (email address) and password, and click **Save**.
   - Repeat for each user.
   - Click **Next** when done.
6. On the **Summary** page, copy and save the **JWT Secret** and the **PostgREST configuration snippet**. Click **Next**.
7. The wizard is complete. You can close the admin tool.

## Step 4: Configure the Neon Data API to Use the pnanon Role

Neon's Data API needs to know which role to use for unauthenticated requests. The admin tool created the `pnanon` role in Step 3.

1. In the Neon dashboard, navigate to **Settings > Data API**.
2. Set the **Anonymous Role** to `pnanon`.
3. Set the **JWT Secret** to the value copied from the admin tool's Summary page.
4. Save the settings.

The Data API is now configured to use the roles and security policies the admin tool created.

## Step 5: Configure Cloud Sync in Project Notes

Each user configures Project Notes with their own credentials:

1. Open Project Notes.
2. From the **File** menu, choose **Cloud Sync Settings...**
3. Check **Sync data with cloud host**.
4. Set **Sync Host Type** to **Self-Hosted PostgREST**.
5. Enter the **Server URL** — the Neon Data API URL from Step 2 (e.g., `https://ep-xxxx.us-east-2.aws.neon.tech/v1`).
6. Enter your **Username (Email)** and **Password** — the credentials created for this user in the admin tool.
7. Optionally enter an **Encryption Phrase** to encrypt data at rest on the server. Store this phrase safely — it cannot be recovered, and every machine syncing to this project must use the same phrase.
8. Click **OK**.

Project Notes will verify the connection. If it succeeds, sync will begin automatically.

## Step 6: Verify Sync Is Working

1. After saving settings, observe the sync progress bar in the bottom-right corner of the status bar.
2. The bar fills as records are pushed to Neon and disappears when sync is complete.
3. To force a full sync, choose **File > Sync All**.

If the connection fails, a warning dialog will appear. Verify the Data API URL, JWT Secret configuration, and user credentials.

## Managing Users

To add, remove, or change passwords for users after initial setup:

1. Open the **Project Notes Remote Host** admin tool.
2. On the **Connection** page, enter your Neon PostgreSQL credentials and check **Already set up**, then click **Next** twice to skip to the **Users** page.
3. Add, edit, or remove users as needed.

## Notes and Limitations

- Neon databases scale to zero after a period of inactivity. The first sync after a cold start may take a few extra seconds while the database wakes up.
- The **Encryption Phrase** encrypts data before it leaves Project Notes. All machines syncing to the same project must use the same phrase.
- The JWT Secret must match between the Neon Data API configuration and the value generated by the admin tool. If you regenerate the JWT secret, update the Neon Data API settings and run **File > Sync All** on all connected machines.

## Related Pages

- [Remote Host](<../InterfaceOverview/RemoteHost.md>) — Full feature reference for all Cloud Sync settings and behaviors
- [Setting Up Supabase](<CloudSync_Supabase.md>)
- [Setting Up a Self-Hosted PostgREST Server](<CloudSync_SelfHosted.md>)
- [File Menu](<../InterfaceOverview/FileMenu.md>) — Cloud Sync Settings dialog reference

# Remote Host

The Remote Host feature synchronizes your local Project Notes database with a cloud-hosted PostgreSQL server. This lets you access your project data from multiple machines, collaborate with a team, and keep everything in sync automatically in the background.

Cloud sync in Project Notes requires two things: a configured remote host (set up using the **Project Notes Remote Host** admin tool) and the connection settings entered in **File > Cloud Sync Settings...**

## Project Notes Remote Host Admin Tool

The **Project Notes Remote Host** application is a separate wizard-based tool used by administrators to set up and manage the server-side infrastructure. It handles database schema creation, user management, and configuration — no manual SQL or server administration is required.

The admin tool supports two hosting modes:
- **Self-Hosted** — a PostgreSQL database you run on your own server or VM, with PostgREST in front of it
- **Supabase** — a managed Supabase project (includes PostgreSQL and PostgREST built in)

For Neon, use **Self-Hosted** mode when running the admin tool and point it at your Neon PostgreSQL connection string.

### Wizard Overview

The admin tool walks through a six-page wizard:

#### Page 1 — Connection

Enter your database credentials and select the hosting mode.

**Self-Hosted:**
- PostgreSQL host, port, and database name
- Superuser username and password
- A "Test Connection" button verifies credentials before proceeding

**Supabase:**
- Supabase Project URL
- Service Role Key (from **Supabase Dashboard > Settings > API**)
- A "Test Connection" button verifies credentials before proceeding

If you are reconnecting to a database that has already been set up, check **Already set up** to skip the installation steps and go directly to user management.

#### Page 2 — Configure

Displays a summary of your connection details for review before setup begins. Confirm the host, database name, and superuser, then continue.

#### Page 3 — Install

Runs the 21-step database setup sequence on a background thread. Each step is shown in a progress log with a success or failure indicator.

What the setup creates:
- **PostgreSQL roles** — `pnauthenticator` (PostgREST authenticator), `pnanon` (anonymous access), and `pnapp_user` (authenticated app access), with appropriate role grants
- **Schema usage grants** — PostgREST roles are granted access to the public schema
- **JWT helper functions** — functions to encode and sign JWT tokens (self-hosted only; Supabase manages its own JWT infrastructure)
- **`auth_users` table** — stores bcrypt-hashed passwords for user authentication (self-hosted only; Supabase Auth handles this in hosted mode)
- **`sync_data` table** — the central table where all synchronized records are stored, with an index on `(userid, tablename, updateddate)` for efficient pull queries
- **Row-level security (RLS)** — policies on `sync_data` so each user can only read and write their own records
- **`rpc_login` function** — a PostgREST RPC endpoint that validates credentials and returns a JWT token (self-hosted only)

In **Self-Hosted** mode, all steps are required — setup aborts if any step fails. In **Supabase** mode, steps that conflict with Supabase's managed infrastructure (JWT functions, `auth_users` table) are run as soft steps that log a warning and continue if they fail.

You can cancel setup at any time using the Cancel button; completed steps are not rolled back automatically. Use the Teardown page to remove all objects if you need to start over.

#### Page 4 — Users

Add, edit, and remove users who are authorized to sync data. Each user authenticates independently and can only see their own synced records.

- **Add User** — opens a dialog to enter a username and password; the user is created as a PostgreSQL role (self-hosted) or as a Supabase Auth user (Supabase), and added to the user table
- **Edit User** — change a user's password
- **Remove User** — deletes the user's login credentials and removes them from the user table; their sync data is not deleted from the server

The Users table displays each username and the date the account was created.

#### Page 5 — Summary

Displays the secrets and PostgREST configuration needed to complete the server setup. Copy these values before closing the admin tool.

**Self-Hosted only:**
- **Authenticator Password** — the password generated for the `pnauthenticator` role
- **JWT Secret** — the secret used to sign and verify JWT tokens; must match the `jwt-secret` value in your PostgREST configuration

**PostgREST configuration snippet** — a ready-to-paste `postgrest.conf` block containing:
```ini
db-uri        = "postgresql://pnauthenticator:<password>@host/database"
db-schema     = "public"
db-anon-role  = "pnanon"
jwt-secret    = "<generated secret>"
server-host   = "0.0.0.0"
server-port   = 3000
```

Copy the `jwt-secret` and `authenticatorPassword` values to a secure location. They are displayed only once.

#### Page 6 — Teardown

Removes all database objects created by the setup: roles, functions, tables, and RLS policies. Use this if you need to completely uninstall the sync infrastructure from the database.

Teardown continues even if individual steps fail, so it cleans up as much as possible even in a partially-configured database.

> **Warning:** Teardown is irreversible. All synced data in `sync_data` will be deleted along with the schema objects.

---

## Cloud Sync Settings in Project Notes

Once the remote host is set up with the admin tool, configure the connection in Project Notes under **File > Cloud Sync Settings...**

### Sync data with cloud host

Enables or disables cloud sync entirely. When unchecked, the local database operates in standalone mode and no data is sent to or received from the remote server. The sync progress bar is hidden when sync is disabled.

### Sync Host Type

| Option | Description |
| :--- | :--- |
| **Self-Hosted PostgREST** | A PostgREST server you run yourself, pointing at any PostgreSQL database |
| **Supabase** | A managed Supabase project — requires an additional Anon Key field |

For setup instructions, see:
- [Setting Up Supabase](<../Introduction/CloudSync_Supabase.md>)
- [Setting Up Neon](<../Introduction/CloudSync_Neon.md>)
- [Setting Up a Self-Hosted PostgREST Server](<../Introduction/CloudSync_SelfHosted.md>)

### Server URL

The base URL of your PostgREST endpoint. Do not include a trailing slash.

- **Self-Hosted PostgREST:** address and port of your PostgREST server (e.g., `http://192.168.1.100:3000` or `https://sync.yourcompany.com`)
- **Supabase:** your Supabase Project URL (e.g., `https://abcdefgh.supabase.co`)
- **Neon:** your Neon Data API endpoint URL

### Username (Email)

The email address used to authenticate with the remote host. Must match a user account created in the admin tool.

### Password

The password for the account identified by Username. Stored in your local OS profile and never synced.

### Encryption Phrase

An optional passphrase that encrypts data before it is sent to the server. The server stores only ciphertext — the phrase never leaves your machine.

**Important:**
- If you lose the phrase, data on the server cannot be decrypted. There is no recovery mechanism.
- Every machine syncing to the same server must use the **same encryption phrase**.
- Leave blank to disable encryption. Data is still protected in transit when HTTPS is configured on the server.
- Changing the phrase requires a **Sync All** to re-push all records in the new encrypted state.

### Supabase Anon Key

Appears only when **Sync Host Type** is **Supabase**. Enter the `anon` (public) key from **Supabase Dashboard > Settings > API > Project API Keys**. Do not use the `service_role` key.

---

## Sync All

**File > Sync All** resets all sync flags and re-synchronizes the entire database with the remote host from scratch.

Use Sync All when:
- Records appear stuck and the sync progress bar does not clear
- You connect a new machine and want to pull all existing data immediately
- You change the Encryption Phrase and need to re-push all records
- You suspect the remote database is out of sync after a connectivity interruption

Sync All can take several minutes for large databases. The sync progress bar shows overall progress. Project Notes remains fully usable while Sync All runs in the background.

**To run Sync All:** From the **File** menu, choose **Sync All**.

---

## Sync Status Bar

When cloud sync is active, a progress bar appears in the bottom-right corner of the status bar.

- **While syncing** — the bar fills from left to right as records are pushed and pulled. Hover over the bar to see the exact percentage and the number of records pending push and pull.
- **When complete** — the bar disappears automatically once the database is fully synchronized.
- **When sync is disabled or not configured** — the bar is hidden.

---

## Automatic Background Sync

Project Notes runs a background sync process on a short interval while the application is open. Each cycle:

1. Checks for local records created, modified, or deleted since the last sync
2. Pushes those changes to the remote host via the PostgREST API
3. Pulls any changes made by other users on the same remote host
4. Merges remote changes into the local database

No manual action is needed for routine sync — it runs continuously in the background.

## Conflict Handling

When the same record is modified on two machines before either has synced, Project Notes uses last-write-wins based on modification timestamps. To minimize conflicts in team environments, sync frequently and avoid extended offline periods.

## Working Offline

If the remote host is unreachable, Project Notes continues to function using the local database. Changes made while offline are flagged and pushed automatically when the connection is restored.

---

## Troubleshooting

### "Cannot connect to server" warning on save

- Verify the Server URL is correct and includes the protocol (`http://` or `https://`).
- Confirm PostgREST is running and listening on the expected port.
- Check that firewalls or VPNs are not blocking the connection.
- For Supabase or Neon, verify the project URL and credentials are correct.

### Sync progress bar stays visible for a long time

- A large number of pending records may be in the queue. Wait for completion or run **File > Sync All** to reset.
- Review **View > Logs** for sync-related error messages.

### Records appear on one machine but not another

- Confirm both machines have the same Server URL configured.
- Run **File > Sync All** on both machines.
- Verify both machines use the same Encryption Phrase.

### Encryption phrase mismatch

Records pushed with one phrase cannot be read with a different phrase. To resolve: ensure all machines use the identical Encryption Phrase, then run **File > Sync All** on each machine.

---

## Related Pages

- [Setting Up Supabase](<../Introduction/CloudSync_Supabase.md>)
- [Setting Up Neon](<../Introduction/CloudSync_Neon.md>)
- [Setting Up a Self-Hosted PostgREST Server](<../Introduction/CloudSync_SelfHosted.md>)
- [File Menu](<FileMenu.md>) — Cloud Sync Settings and Sync All reference
- [Getting Started](<../Introduction/GettingStarted.md>)

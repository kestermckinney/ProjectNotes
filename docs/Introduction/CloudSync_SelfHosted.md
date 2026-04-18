# Setting Up a Self-Hosted PostgREST Server

Self-hosting gives you complete control over your data, network topology, and infrastructure costs. This guide walks through setting up PostgreSQL and PostgREST on a server you control — whether that is a local VM, a corporate server, or a cloud VM (AWS EC2, Azure VM, DigitalOcean Droplet, etc.).

The **Project Notes Remote Host** admin tool handles all database schema setup and user management. You do not need to run any SQL manually.

When you are done with setup, see [Remote Host](<../InterfaceOverview/RemoteHost.md>) for a full reference of all sync features and settings.

## Architecture Overview

```
Project Notes  →  PostgREST  →  PostgreSQL
                  (port 3000)   (port 5432)
```

Both PostgREST and PostgreSQL can run on the same server or on separate servers. Running them together on a single server is the simplest setup for small teams.

## Prerequisites

- A server running Linux (Ubuntu 22.04 LTS recommended), macOS, or Windows Server
- PostgreSQL 14 or later installed and running
- The **Project Notes Remote Host** admin tool installed
- Project Notes installed and running on at least one machine

## Step 1: Install PostgreSQL

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y postgresql postgresql-contrib
sudo systemctl enable postgresql
sudo systemctl start postgresql
```

### macOS (Homebrew)

```bash
brew install postgresql@16
brew services start postgresql@16
```

### Windows

Download and run the PostgreSQL installer from [postgresql.org](https://www.postgresql.org/download/windows/). During installation, set a password for the `postgres` superuser and note the port (default: 5432).

## Step 2: Create the Project Notes Database

Connect to PostgreSQL as the superuser and create the database:

```bash
sudo -u postgres psql
```

```sql
CREATE DATABASE project_notes;
\q
```

## Step 3: Run the Admin Tool

The **Project Notes Remote Host** admin tool initializes the schema, creates all required roles, and sets up user accounts. Run it once from any machine that can reach your PostgreSQL server.

1. Open the **Project Notes Remote Host** application.
2. On the **Connection** page:
   - Select **Self-Hosted** as the hosting mode.
   - Enter your PostgreSQL **host**, **port**, **database name** (`project_notes`), **superuser** (`postgres`), and **password**.
   - Click **Test Connection** to verify, then click **Next**.
3. On the **Configure** page, review the connection details and click **Next**.
4. On the **Install** page, the tool runs through 21 setup steps — creating roles (`pnauthenticator`, `pnanon`, `pnapp_user`), the sync table, row-level security policies, JWT helper functions, and the `rpc_login` endpoint. Watch the progress log and wait for setup to complete, then click **Next**.
5. On the **Users** page, add each person who will sync Project Notes data:
   - Click **Add User**, enter a username (email address) and password, and click **Save**.
   - Repeat for each user.
   - Click **Next** when done.
6. On the **Summary** page, copy and save the **Authenticator Password** and **JWT Secret** — these are displayed only once and are required to configure PostgREST. Also copy the complete **PostgREST configuration snippet**. Click **Next**.
7. The wizard is complete. You can close the admin tool.

## Step 4: Install and Configure PostgREST

PostgREST is a standalone binary with no runtime dependencies.

### Download PostgREST

Download the latest release for your platform from [postgrest.org](https://postgrest.org/en/stable/install.html) and place the binary in your PATH (e.g., `/usr/local/bin/postgrest`).

### Create the Configuration File

Paste the PostgREST configuration snippet copied from the admin tool's Summary page into a file named `postgrest.conf`. It will look like:

```ini
db-uri        = "postgresql://pnauthenticator:<password>@localhost:5432/project_notes"
db-schema     = "public"
db-anon-role  = "pnanon"
jwt-secret    = "<jwt secret from admin tool>"
server-host   = "0.0.0.0"
server-port   = 3000
```

### Test the Configuration

Run PostgREST manually first to confirm it connects:

```bash
postgrest /etc/postgrest/postgrest.conf
```

Open `http://your-server:3000/` in a browser. You should see a JSON object listing the available tables. If you see an error, check the terminal output for connection or permission issues.

## Step 5: Run PostgREST as a Service

### Linux (systemd)

Create `/etc/systemd/system/postgrest.service`:

```ini
[Unit]
Description=PostgREST API Server
After=network.target postgresql.service

[Service]
ExecStart=/usr/local/bin/postgrest /etc/postgrest/postgrest.conf
Restart=always
RestartSec=5
User=www-data
Group=www-data

[Install]
WantedBy=multi-user.target
```

Enable and start the service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable postgrest
sudo systemctl start postgrest
sudo systemctl status postgrest
```

### macOS (launchd)

Create `~/Library/LaunchAgents/com.postgrest.plist`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.postgrest</string>
    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/postgrest</string>
        <string>/etc/postgrest/postgrest.conf</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
</dict>
</plist>
```

Load it:

```bash
launchctl load ~/Library/LaunchAgents/com.postgrest.plist
```

### Windows (NSSM)

Use [NSSM (Non-Sucking Service Manager)](https://nssm.cc) to wrap PostgREST as a Windows service:

```cmd
nssm install PostgREST "C:\Program Files\PostgREST\postgrest.exe"
nssm set PostgREST AppParameters "C:\PostgREST\postgrest.conf"
nssm start PostgREST
```

## Step 6: Configure HTTPS (Recommended for Internet-Facing Servers)

For servers accessible over the internet, place PostgREST behind a reverse proxy with TLS. Example with **Caddy** (automatically manages certificates):

```
# Caddyfile
sync.yourcompany.com {
    reverse_proxy localhost:3000
}
```

Or with **nginx**:

```nginx
server {
    listen 443 ssl;
    server_name sync.yourcompany.com;

    ssl_certificate     /etc/ssl/certs/yourcompany.crt;
    ssl_certificate_key /etc/ssl/private/yourcompany.key;

    location / {
        proxy_pass http://localhost:3000;
        proxy_set_header Host $host;
    }
}
```

After adding HTTPS, use `https://sync.yourcompany.com` as the Server URL in Project Notes.

## Step 7: Configure Cloud Sync in Project Notes

Each user configures Project Notes with their own credentials:

1. Open Project Notes.
2. From the **File** menu, choose **Cloud Sync Settings...**
3. Check **Sync data with cloud host**.
4. Set **Sync Host Type** to **Self-Hosted PostgREST**.
5. Enter the **Server URL** — the address of your PostgREST server (e.g., `http://192.168.1.100:3000` or `https://sync.yourcompany.com`).
6. Enter your **Username (Email)** and **Password** — the credentials created for this user in the admin tool.
7. Optionally enter an **Encryption Phrase** to encrypt data at rest on the server. Store this phrase safely — it cannot be recovered, and every machine syncing to this server must use the same phrase.
8. Click **OK**.

Project Notes will verify the connection. If it succeeds, sync will begin automatically.

## Step 8: Verify Sync Is Working

1. After saving settings, observe the sync progress bar in the bottom-right corner of the status bar.
2. The bar fills as records are pushed to your server and disappears when sync is complete.
3. To force a full sync, choose **File > Sync All**.

## Managing Users

To add, remove, or change passwords for users after initial setup:

1. Open the **Project Notes Remote Host** admin tool.
2. On the **Connection** page, enter your PostgreSQL superuser credentials and check **Already set up**, then click **Next** twice to skip to the **Users** page.
3. Add, edit, or remove users as needed.

## Firewall and Security Checklist

- Open the PostgREST port (default 3000) only to IP ranges that need access.
- Do not expose PostgreSQL port 5432 directly to the internet — keep it localhost-only or restrict it to the PostgREST server's IP.
- Use HTTPS for any internet-facing PostgREST endpoint.
- Back up the PostgreSQL database regularly (`pg_dump project_notes > backup.sql`).

## Notes and Limitations

- The **Encryption Phrase** encrypts data before it leaves Project Notes. All machines syncing to the same server must use the same phrase.
- PostgREST is stateless — restarting it does not affect the database or sync state.
- If you need to regenerate the JWT secret, rerun the admin tool teardown and setup, update `postgrest.conf`, restart PostgREST, and run **File > Sync All** on all connected machines.

## Related Pages

- [Remote Host](<../InterfaceOverview/RemoteHost.md>) — Full feature reference for all Cloud Sync settings and behaviors
- [Setting Up Supabase](<CloudSync_Supabase.md>)
- [Setting Up Neon](<CloudSync_Neon.md>)
- [File Menu](<../InterfaceOverview/FileMenu.md>) — Cloud Sync Settings dialog reference

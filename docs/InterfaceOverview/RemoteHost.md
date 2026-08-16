# Cloud Sync

Cloud sync keeps your local Project Notes database backed up and synchronized across all of your devices — your desktop installations and the [Project Notes Mobile](../Mobile/ProjectNotesMobile.md) app. It runs automatically in the background while Project Notes is open.

Cloud sync is provided by a **Project Notes Pro** subscription. The hosting is fully managed — there is no server to install, no database to administer, and no host address or API key to enter. You simply sign in with your subscription account and your data is synchronized to Project Notes Pro hosting.

Manage your subscription, billing, and account at [www.projectnotespro.com](https://www.projectnotespro.com).

## Configuring Cloud Sync

Configure the connection under the **Cloud Sync** section of Settings — click the **Settings** icon in the icon rail (or choose **Preferences** from the app menu, the menu icon at the top of the icon rail).

### Enable cloud sync

The master on/off switch for cloud sync. When unchecked, the local database operates in standalone mode and no data is sent to or received from Project Notes Pro hosting. The sync progress line under the status row is hidden when sync is disabled.

### Sync Email

The email address for your Project Notes Pro account.

### Sync Password

The password for your Project Notes Pro account. It is stored in your local OS profile and is never synced.

### Encryption Phrase

An optional passphrase that encrypts your data before it leaves your machine. The server stores only ciphertext — the phrase never leaves your device.

**Important:**

- If you lose the phrase, data on the server cannot be decrypted. There is no recovery mechanism.
- Every device syncing the same account must use the **same encryption phrase**.
- Leave blank to disable encryption. Data is still protected in transit by HTTPS.
- Changing the phrase requires a **Sync All** to re-push all records in the new encrypted state.

### Checking Your Settings

Each field saves as soon as you leave it, so a mistyped password or a mismatched encryption phrase would otherwise sit unnoticed until you wondered why nothing was syncing. To prevent that, Project Notes checks the Cloud Sync fields whenever you change one and then navigate away from Settings. It signs in to Project Notes Pro with the email and password you entered, and confirms that your encryption phrase opens the records already stored for your account.

If something is wrong, a dialog appears before you leave the page:

- **Credentials rejected** — the email and password were not accepted. Sync will not run until they are corrected.
- **Encryption phrase mismatch** — sign-in worked, but none of your stored records can be decrypted with the phrase you entered. Records synced from your other devices would be skipped. (See the Encryption Phrase notes above — every device on the account must use the same phrase.)
- **Host unreachable** — your settings were saved but could not be checked because Project Notes Pro could not be reached. They will be checked again the next time sync runs.

Choose **Back to Settings** to stay and fix the field, or **Leave Anyway** to continue — your entries stay saved either way. The check only runs when you have actually changed one of the Cloud Sync fields.

### Subscription Status

The Cloud Sync section displays the current state of your Project Notes Pro subscription as a status line below the sync icon — for example the plan name and whether the subscription is active.

If your subscription lapses, Project Notes will notify you that the subscription has expired and pause syncing until it is renewed at [www.projectnotespro.com](https://www.projectnotespro.com). Your local data remains fully accessible while a subscription is inactive.

---

## Sync Now vs. Sync All

Project Notes offers two ways to trigger a sync cycle by hand, both reachable from the app menu (File group) and from **Settings > Cloud Sync**:

### Sync Now

**Sync Now** nudges an immediate, incremental sync cycle — the same kind of cycle that already runs automatically in the background. It pushes any local changes and pulls any remote changes since the last cycle. Use it when you want to sync sooner rather than waiting for the next automatic cycle.

**To run Sync Now:** Choose **Sync Now** from the app menu, click **Sync Now** in **Settings > Cloud Sync**, or click the sync icon in the icon rail.

### Sync All

**Sync All** resets all sync flags and performs a full re-push and re-pull of the entire database with Project Notes Pro hosting from scratch.

Use Sync All when:

- Records appear stuck and the sync indicator does not clear
- You connect a new device and want to pull all existing data immediately
- You change the Encryption Phrase and need to re-push all records
- You suspect the remote data is out of sync after a connectivity interruption

Sync All can take several minutes for large databases. Project Notes remains fully usable while Sync All runs in the background.

**To run Sync All:** Choose **Sync All** from the app menu, or click **Sync All** in **Settings > Cloud Sync**.

---

## Sync Indicator

When cloud sync is active, a cloud icon near the bottom of the icon rail shows the current state:

- **While syncing** — the icon spins and a thin circular progress ring fills in around it as records are pushed and pulled. Hover over it to see the exact detail text (percentage and record counts) as a tooltip.
- **When complete** — the icon settles to a plain "synced" cloud.
- **On a network error** — the icon switches to a "cloud off" state.
- **When sync is disabled or not configured** — the icon is dimmed and shows no ring.

Click the sync icon at any time to trigger **Sync Now**.

---

## Sync Stats Window

For a more detailed view of sync activity than the icon-rail indicator provides, click **Sync Stats** in **Settings > Cloud Sync**.

The window is provided by the underlying SqliteSyncPro engine and shows:

- The current high-watermark timestamp used by the pull cursor
- Counts of records pending push and pending pull, broken out by table
- Any recent sync errors and the timestamp of the last successful sync cycle

The **Sync Stats** button is only enabled once the sync engine has been initialized — that is, after cloud sync has been configured and the first sync cycle has started.

---

## Automatic Background Sync

Project Notes runs a background sync process on a short interval while the application is open. The sync engine starts asynchronously after the main window appears, so application startup is not blocked while it initializes — the first sync cycle runs shortly after the app is usable rather than during launch.

Each sync cycle:

1. Checks for local records created, modified, or deleted since the last sync
2. Pushes those changes to Project Notes Pro hosting
3. Pulls any changes made on your other devices
4. Merges remote changes into the local database

No manual action is needed for routine sync — it runs continuously in the background.

## Conflict Handling

When the same record is modified on two devices before either has synced, Project Notes uses last-write-wins based on modification timestamps. To minimize conflicts, sync frequently and avoid extended offline periods.

## Working Offline

If Project Notes Pro hosting is unreachable, Project Notes continues to function using the local database. Changes made while offline are flagged and pushed automatically when the connection is restored.

---

## Troubleshooting

### "Cannot connect" or sign-in warning on save

- Verify your **Sync Email** and **Sync Password** are correct.
- Confirm your subscription is active at [www.projectnotespro.com](https://www.projectnotespro.com).
- Check that a firewall or VPN is not blocking the connection.

### Sync indicator stays active for a long time

- A large number of pending records may be in the queue. Wait for completion or run **Sync All** to reset.
- Open the **Log Viewer** from the app menu and review `syncerrors.log` for sync-related error messages (see [Error Log](<ErrorLog.md>)).

### Records appear on one device but not another

- Confirm both devices are signed in with the same Project Notes Pro account.
- Run **Sync All** on both devices.
- Verify both devices use the same Encryption Phrase.

### Encryption phrase mismatch

Records pushed with one phrase cannot be read with a different phrase. To resolve: ensure all devices use the identical Encryption Phrase, then run **Sync All** on each device.

---

## Related Pages

- [Getting Started](../Introduction/GettingStarted.md)
- [Project Notes Mobile](../Mobile/ProjectNotesMobile.md)

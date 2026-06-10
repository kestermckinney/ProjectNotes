# Cloud Sync

Cloud sync keeps your local Project Notes database backed up and synchronized across all of your devices — your desktop installations and the [Project Notes Mobile](../Mobile/ProjectNotesMobile.md) app. It runs automatically in the background while Project Notes is open.

Cloud sync is provided by a **Project Notes Pro** subscription. The hosting is fully managed — there is no server to install, no database to administer, and no host address or API key to enter. You simply sign in with your subscription account and your data is synchronized to Project Notes Pro hosting.

Manage your subscription, billing, and account at [www.projectnotespro.com](https://www.projectnotespro.com).

## Configuring Cloud Sync

Configure the connection in Project Notes under **File > Cloud Sync Settings...**

### Sync all your devices and backup your data

The master on/off switch for cloud sync. When unchecked, the local database operates in standalone mode and no data is sent to or received from Project Notes Pro hosting. The sync progress bar is hidden when sync is disabled.

### Username (Email)

The email address for your Project Notes Pro account.

### Password

The password for your Project Notes Pro account. It is stored in your local OS profile and is never synced.

### Encryption Phrase

An optional passphrase that encrypts your data before it leaves your machine. The server stores only ciphertext — the phrase never leaves your device.

**Important:**

- If you lose the phrase, data on the server cannot be decrypted. There is no recovery mechanism.
- Every device syncing the same account must use the **same encryption phrase**.
- Leave blank to disable encryption. Data is still protected in transit by HTTPS.
- Changing the phrase requires a **Sync All** to re-push all records in the new encrypted state.

### Subscription Status

The dialog displays the current state of your Project Notes Pro subscription — for example the plan name and whether the subscription is active. If you open the dialog before a database is connected, it shows *"Not connected — open a database to view subscription status."*

Below the status line, a **Project ID** indicates which Project Notes Pro project your data is associated with, along with the environment it is connecting to.

If your subscription lapses, Project Notes will notify you that the subscription has expired and pause syncing until it is renewed at [www.projectnotespro.com](https://www.projectnotespro.com). Your local data remains fully accessible while a subscription is inactive.

---

## Sync All

**File > Sync All** resets all sync flags and re-synchronizes the entire database with Project Notes Pro hosting from scratch.

Use Sync All when:

- Records appear stuck and the sync progress bar does not clear
- You connect a new device and want to pull all existing data immediately
- You change the Encryption Phrase and need to re-push all records
- You suspect the remote data is out of sync after a connectivity interruption

Sync All can take several minutes for large databases. The sync progress bar shows overall progress. Project Notes remains fully usable while Sync All runs in the background.

**To run Sync All:** From the **File** menu, choose **Sync All**.

---

## Sync Status Bar

When cloud sync is active, a progress bar appears in the bottom-right corner of the status bar.

- **While syncing** — the bar fills from left to right as records are pushed and pulled. Hover over the bar to see the exact percentage and the number of records pending push and pull.
- **When complete** — the bar disappears automatically once the database is fully synchronized.
- **When sync is disabled or not configured** — the bar is hidden.

---

## Sync Stats Window

For a more detailed view of sync activity than the status-bar progress bar provides, you can open the **Sync Stats** window from **View > Sync Stats**. The menu item is a checkable toggle: select it to show the window, select it again (or close the window) to hide it.

The window is provided by the underlying SqliteSyncPro engine and shows:

- The current high-watermark timestamp used by the pull cursor
- Counts of records pending push and pending pull, broken out by table
- Any recent sync errors and the timestamp of the last successful sync cycle

The Sync Stats menu item is only enabled once the sync engine has been initialized — that is, after Cloud Sync Settings have been configured and the first sync cycle has started.

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

- Verify your **Username (Email)** and **Password** are correct.
- Confirm your subscription is active at [www.projectnotespro.com](https://www.projectnotespro.com).
- Check that a firewall or VPN is not blocking the connection.

### Sync progress bar stays visible for a long time

- A large number of pending records may be in the queue. Wait for completion or run **File > Sync All** to reset.
- Review **View > Logs** for sync-related error messages.

### Records appear on one device but not another

- Confirm both devices are signed in with the same Project Notes Pro account.
- Run **File > Sync All** on both devices.
- Verify both devices use the same Encryption Phrase.

### Encryption phrase mismatch

Records pushed with one phrase cannot be read with a different phrase. To resolve: ensure all devices use the identical Encryption Phrase, then run **File > Sync All** on each device.

---

## Related Pages

- [File Menu](FileMenu.md) — Cloud Sync Settings and Sync All reference
- [Getting Started](../Introduction/GettingStarted.md)
- [Project Notes Mobile](../Mobile/ProjectNotesMobile.md)

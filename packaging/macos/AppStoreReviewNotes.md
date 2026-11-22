# Mac App Store review notes

Notes for App Review and for whoever submits the Mac App Store build. The build comes from `packaging/macos/build_appstore.sh`, which configures with `-DPN_APP_STORE=ON`. `AppStoreReleasing.md` covers the certificates, profile and upload.

## What reviewers should know

**Plugins.** Project Notes can be extended with JavaScript plugins. The app never downloads code: the plugins inside the app bundle ship with it and are reviewed with it, and a user can add plugins only by choosing a `.pnplugin` file or folder they already have, in Settings > Plugins > Install. A plugin the user adds stays off until the user reviews the list of what it may do (which records it can read or change, which web sites it can reach) and turns it on. Plugins run inside the app's own JavaScript engine under the same sandbox as the app; they cannot start other programs, and they cannot read files outside the folders the user picked. This is the same arrangement scripting applications use for user scripts (guideline 2.5.2).

If review asks for user plugins to be removed, the build option `PN_APP_STORE_USER_PLUGINS=OFF` turns them off without other changes; the bundled plugins keep working.

**Updates.** The in-app updater is left out; Help > Check for Updates does not appear.

**Other programs.** The App Store build does not start other programs. The Thunderbird integration and the Script Editor command are absent.

## Entitlements

| Entitlement | Why |
| --- | --- |
| `app-sandbox` | Required. |
| `cs.allow-jit` | Qt's QML engine and the plugin JavaScript engine compile scripts to native code. |
| `network.client` | Cloud sync with Project Notes Pro, Office 365 (Microsoft Graph), iCloud contacts (CardDAV), and web requests plugins make to the hosts they declare. |
| `network.server` | Some sign-ins (an organization's Keycloak server, for example) finish by redirecting the browser to `http://localhost:<port>`. The app listens on the loopback address only, only while a sign-in is waiting. |
| `files.user-selected.read-write`, `files.bookmarks.app-scope` | Project folders, File Finder search locations and file locations the user picks are reopened in later sessions through security-scoped bookmarks. On the first start the app also offers to copy the data of the direct download (sold separately, not sandboxed); the user picks that version's data folder in an Open dialog, and the app only reads it. |
| `personal-information.calendars` | The "MacOS Email and Calendar" integration saves meetings the user schedules to their calendar through EventKit, without attendees, so the user adds invitees and sends from Calendar. Mail drafts go through the system's compose-email sharing service and need no entitlement. |
| `personal-information.addressbook` | Optional contact synchronization with the Contacts app. |

The usage descriptions for Calendars and Contacts are in `Info.plist`.

The app sends no Apple Events and uses no temporary exceptions. The 6.5.1 submission was rejected under Guideline 2.4.5(i) for `temporary-exception.apple-events`. Calendar events now go through EventKit (`MacCalendarBackend`) in both the App Store build and the direct download. The read-only exception for the direct download's data is gone too: the user picks that folder in an Open dialog instead. `build_appstore.sh` refuses to package a build whose signature carries an Apple Events or temporary-exception entitlement.

## Before submitting

- The Mac build ships in the iOS app's record ("Project Notes Pro") under its bundle ID, `com.projectnotespro.projectnotesmobile`, so one listing and one purchase cover both. The direct download uses the same bundle ID; it is not sandboxed and keeps its data in `~/Library`.
- The data lives in the sandbox container (`~/Library/Containers/com.projectnotespro.projectnotesmobile/Data/`). On the first start, `DirectDownloadMigration` offers to copy the direct download's database, plugins and settings into it, from the folder the user picks. The sandbox cannot read `~/Library/Preferences`, so the direct download writes a copy of its settings to `AppStoreMigration/` in its data folder at start and quit (`DirectDownloadSettingsExport.cpp`). A direct download too old to do that leaves only its data; settings must be set again, or brought over with Export Settings.
- Paths typed by hand, or stored in a database copied from another computer, are not reachable until the user picks the folder once (Files & Folders > Browse Folder, or Settings > File Finder).
- Export compliance: the app uses HTTPS and the operating system's TLS. Cloud sync also encrypts data with the user's passphrase before upload; answer the encryption questions in App Store Connect accordingly.
- Licensing: the project is GPL-3.0 and the maintainer holds the copyright. Check any contributed GPL code and third-party libraries (Qt is used under the LGPL) before submitting.

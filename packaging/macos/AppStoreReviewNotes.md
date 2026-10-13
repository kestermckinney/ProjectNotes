# Mac App Store review notes

Notes for App Review and for whoever submits the Mac App Store build. The build comes from `cmake -DPN_APP_STORE=ON` and `packaging/macos/build_appstore.sh`.

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
| `files.user-selected.read-write`, `files.bookmarks.app-scope` | Project folders, File Finder search locations and file locations the user picks are reopened in later sessions through security-scoped bookmarks. |
| `automation.apple-events`, `temporary-exception.apple-events` (Calendar, Mail, System Events) | The "MacOS Email and Calendar" integration creates draft messages in Mail and draft events in Calendar, which the user reviews and sends. Nothing is sent without the user. |
| `personal-information.calendars` | Calendar requires calendar access from any app that scripts it. |
| `personal-information.addressbook` | Optional contact synchronization with the Contacts app. |

The usage descriptions for Apple Events, Calendars and Contacts are in `Info.plist`.

## Before submitting

- The data folder moves into the sandbox container (`~/Library/Containers/com.projectnotespro.ProjectNotes/Data/`), and the app cannot read the direct-download version's data folder on its own. A user switching versions brings their data across with cloud sync, or with Export XML in the old version and Import XML in the new one.
- Paths typed by hand, or stored in a database copied from another computer, are not reachable until the user picks the folder once (Files & Folders > Browse Folder, or Settings > File Finder).
- Export compliance: the app uses HTTPS and the operating system's TLS. Cloud sync also encrypts data with the user's passphrase before upload; answer the encryption questions in App Store Connect accordingly.
- Licensing: the project is GPL-3.0 and the maintainer holds the copyright. Check any contributed GPL code and third-party libraries (Qt is used under the LGPL) before submitting.

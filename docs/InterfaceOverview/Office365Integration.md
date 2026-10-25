# Office 365 Integration

The **Office 365 Integration** settings page connects Project Notes to Microsoft 365 through Microsoft Graph. It is separate from File Finder so the same authenticated connection can support additional Project Notes features in the future.

The **Microsoft 365 drafts** integration creates email drafts and draft calendar events in your mailbox. File Finder discovers project channels in Microsoft Teams and the documents in their SharePoint-backed folders.

Native [Contact Synchronization](Preferences.md#contact-synchronization) and **Full Contact Export** also use this account. Use this tab's single sign-in action to grant `Contacts.ReadWrite` alongside email and calendar access. Contact export remains available when scheduled and manual synchronization are disabled.

## Before You Sign In

Your organization must provide a **dedicated** Microsoft Entra app registration for Office 365 Integration. Do not reuse the app registration configured for the old Outlook Integration plugin, whose consent only covers Mail, Calendar, Contacts, and Tasks scopes. Office 365 Integration requests different, higher-privilege delegated Microsoft Graph scopes:

For email drafts and meeting invitations:

- `User.Read`
- `Mail.ReadWrite`
- `Calendars.ReadWrite`
- `offline_access`

For contact synchronization and Full Contact Export:

- `Contacts.ReadWrite`

Importing contacts without ever exporting them needs only `Contacts.Read`. Project Notes asks for `Contacts.ReadWrite` whenever Office 365 is the contact provider, so grant that one unless you are sure nobody will export.

For File Finder's Teams and SharePoint discovery:

- `Team.ReadBasic.All`
- `Channel.ReadBasic.All`
- `Files.Read.All`

`Mail.ReadWrite` creates and updates drafts. It also satisfies plugins that ask for `Mail.Read`, such as Archive Mail. Project Notes never requests `Mail.Send`, so it cannot send mail from your account.

All of these are **delegated** permissions. Application permissions are not used.

### App registration settings

Project Notes signs in with the device-code flow and has no client secret. On the app registration's **Authentication** page, set **Allow public client flows** to **Yes**. Without it, sign-in fails after you enter the code.

The **Supported account types** setting must agree with the tenant ID you enter in Project Notes. The default tenant, `organizations`, needs a registration that accepts accounts in any organizational directory. A registration limited to your own directory works when you enter your specific tenant ID.

Microsoft Graph marks `Team.ReadBasic.All`, `Channel.ReadBasic.All`, and `Files.Read.All` as requiring administrator consent. A tenant administrator must grant consent for these permissions on the dedicated app registration (Microsoft Entra admin center > **App registrations** > the app > **API permissions** > **Grant admin consent**) before sign-in will succeed. If sign-in appears to complete but File Finder then reports an error mentioning admin approval (AADSTS90094), consent has not been granted yet, or the tenant/client ID entered below points at an app registration that was never granted it.

## Settings

Open the app menu, choose **Settings**, and select **Office 365 Integration**.

| Setting or action | Description |
| :--- | :--- |
| **Microsoft Entra tenant ID** | The directory (tenant) ID for the organization that owns the app registration. The default `organizations` allows sign-in with a work or school account from any Microsoft Entra organization. Use a specific tenant ID when access should be restricted to one organization. |
| **Application (client) ID** | The application ID assigned to the Microsoft Entra app registration. |
| **Sign in to Microsoft 365** | Begins device-code sign-in and requests email, calendar, and contact permissions. Project Notes displays a code and a button that opens Microsoft's sign-in page. Copy the code, open the page, and complete sign-in in your browser. |
| **Sign out** | Removes the saved refresh token and invalidates cached Office 365 File Finder state. It does not remove local File Finder settings or existing **Files & Folders** entries. |

The status panel reports whether Project Notes is signed in and shows the device code and verification-page button while authentication is in progress.

## Credential Storage

Project Notes stores the Microsoft refresh token in the operating system's credential vault. Access tokens remain in memory. The tenant ID, client ID, and connection state are stored in the local Project Notes settings profile and do not sync through the project database. These settings are kept independent of the old Outlook Integration plugin's tenant ID and application ID — changing one never affects the other.

## Use with File Finder

After authentication succeeds:

1. Open **Settings > File Finder**.
2. Enable **File Finder**.
3. Enable **Include Office 365 locations**.
4. Select **Scan Now**.

File Finder enumerates the signed-in user's joined Teams and their channels. A channel is associated with an active project when its name contains the complete project number. It then scans the channel's SharePoint-backed files folder using the classification and folder-exclusion rules from the File Finder page.

File Finder caches Microsoft Graph folder modification times to avoid reading unchanged remote subtrees. Changing classification rules or folder exclusions, signing out, or selecting **Reconsider All Files** clears that cache.

When a file is discovered both locally and through a Teams channel, File Finder keeps the local path on the **Files & Folders** row instead of replacing it with the Teams/SharePoint web link, as long as the local copy is still there. This keeps the row usable from applications that aren't integrated with Microsoft Teams.

For Word, Excel, PowerPoint, and Project files, File Finder stores the direct SharePoint address of the document (for example `.../Shared Documents/General/Budget.xlsx`) rather than the browser viewer page. Other files keep the browser link.

Clicking **Open** on a row with a direct document address launches the matching Office app on that document. The link opens in the browser instead when the app isn't installed (Safari on the mobile app). Viewer pages (`.../_layouts/15/Doc.aspx?...`) and short sharing links (`.../:x:/r/...`) always open in the browser, because Office apps can't open them directly; Office for the web offers **Open in Desktop App** from there. The stored link is never rewritten when you open it.

Rows found by earlier scans still hold viewer-page links. Select **Reconsider All Files** on the File Finder page once to replace them with direct addresses.

See [File Finder](<FileFinder.md>) for discovery, matching, classification settings, and how local/Teams precedence works.

## Use for Email and Meetings

After authentication succeeds:

1. Open **Settings > Email, Contacts, and Calendar**.
2. Set **Integration** to **Microsoft 365 drafts**.

Reports then become drafts in your mailbox and meetings become draft events in your calendar, both opened in Outlook on the web for review. Nothing is sent by Project Notes.

If you signed in before meeting scheduling existed, sign out and sign in again. The earlier sign-in did not request calendar permission.

See [Email and Meeting Setup](<EmailAndMeetingSetup.md>) for the full setup.

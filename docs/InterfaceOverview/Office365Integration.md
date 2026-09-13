# Office 365 Integration

The **Office 365 Integration** settings page connects Project Notes to Microsoft 365 through Microsoft Graph. It is separate from File Finder so the same authenticated connection can support additional Project Notes features in the future.

The current native integration is used by File Finder to discover project channels in Microsoft Teams and documents stored in their SharePoint-backed file folders. Email, meeting, and contact features continue to use the separately configured [Outlook Integration](<OutlookIntegration.md>).

## Before You Sign In

Your organization must provide a **dedicated** Microsoft Entra app registration for Office 365 Integration. Do not reuse the app registration configured for [Outlook Integration](<OutlookIntegration.md>) — its consent only covers Mail, Calendar, Contacts, and Tasks scopes. Office 365 Integration requests different, higher-privilege delegated Microsoft Graph scopes:

- `Team.ReadBasic.All`
- `Channel.ReadBasic.All`
- `Files.Read.All`
- `offline_access`

Microsoft Graph marks `Team.ReadBasic.All`, `Channel.ReadBasic.All`, and `Files.Read.All` as requiring administrator consent. A tenant administrator must grant consent for these permissions on the dedicated app registration (Microsoft Entra admin center > **App registrations** > the app > **API permissions** > **Grant admin consent**) before sign-in will succeed. If sign-in appears to complete but File Finder then reports an error mentioning admin approval (AADSTS90094), consent has not been granted yet, or the tenant/client ID entered below points at an app registration that was never granted it.

## Settings

Open the app menu, choose **Settings**, and select **Office 365 Integration**.

| Setting or action | Description |
| :--- | :--- |
| **Microsoft Entra tenant ID** | The directory (tenant) ID for the organization that owns the app registration. The default `organizations` allows sign-in with a work or school account from any Microsoft Entra organization. Use a specific tenant ID when access should be restricted to one organization. |
| **Application (client) ID** | The application ID assigned to the Microsoft Entra app registration. |
| **Sign in to Microsoft 365** | Begins device-code sign-in. Project Notes displays a code and a button that opens Microsoft's sign-in page. Copy the code, open the page, and complete sign-in in your browser. |
| **Sign out** | Removes the saved refresh token and invalidates cached Office 365 File Finder state. It does not remove local File Finder settings or existing **Files & Folders** entries. |

The status panel reports whether Project Notes is signed in and shows the device code and verification-page button while authentication is in progress.

## Credential Storage

Project Notes stores the Microsoft refresh token in the operating system's credential vault. Access tokens remain in memory. The tenant ID, client ID, and connection state are stored in the local Project Notes settings profile and do not sync through the project database. These settings are kept independent of [Outlook Integration](<OutlookIntegration.md>)'s tenant ID and application ID — changing one never affects the other.

## Use with File Finder

After authentication succeeds:

1. Open **Settings > File Finder**.
2. Enable **File Finder**.
3. Enable **Include Office 365 locations**.
4. Select **Scan Now**.

File Finder enumerates the signed-in user's joined Teams and their channels. A channel is associated with an active project when its name contains the complete project number. It then scans the channel's SharePoint-backed files folder using the classification and folder-exclusion rules from the File Finder page.

File Finder caches Microsoft Graph folder modification times to avoid reading unchanged remote subtrees. Changing classification rules or folder exclusions, signing out, or selecting **Reconsider All Files** clears that cache.

When a file is discovered both locally and through a Teams channel, File Finder keeps the local path on the **Files & Folders** row instead of replacing it with the Teams/SharePoint web link, as long as the local copy is still there. This keeps the row usable from applications that aren't integrated with Microsoft Teams.

See [File Finder](<FileFinder.md>) for discovery, matching, classification settings, and how local/Teams precedence works.

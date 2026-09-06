# Office 365 Integration

The **Office 365 Integration** settings page connects Project Notes to Microsoft 365 through Microsoft Graph. It is separate from File Finder so the same authenticated connection can support additional Project Notes features in the future.

The current native integration is used by File Finder to discover project channels in Microsoft Teams and documents stored in their SharePoint-backed file folders. Email, meeting, and contact features continue to use the separately configured [Outlook Integration](<OutlookIntegration.md>).

## Before You Sign In

Your organization must provide a Microsoft Entra app registration that permits Project Notes to request these delegated Microsoft Graph scopes:

- `Team.ReadBasic.All`
- `Channel.ReadBasic.All`
- `Files.Read.All`
- `offline_access`

Administrator consent may be required by your organization's Microsoft 365 policies.

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

Project Notes stores the Microsoft refresh token in the operating system's credential vault. Access tokens remain in memory. The tenant ID, client ID, and connection state are stored in the local Project Notes settings profile and do not sync through the project database.

## Use with File Finder

After authentication succeeds:

1. Open **Settings > File Finder**.
2. Enable **File Finder**.
3. Enable **Include Office 365 locations**.
4. Select **Scan Now**.

File Finder enumerates the signed-in user's joined Teams and their channels. A channel is associated with an active project when its name contains the complete project number. It then scans the channel's SharePoint-backed files folder using the classification and folder-exclusion rules from the File Finder page.

File Finder caches Microsoft Graph folder modification times to avoid reading unchanged remote subtrees. Changing classification rules or folder exclusions, signing out, or selecting **Reconsider All Files** clears that cache.

See [File Finder](<FileFinder.md>) for discovery, matching, and classification settings.

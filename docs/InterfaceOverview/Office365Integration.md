# Office 365 Integration

The **Office 365 Integration** settings page connects Project Notes to Microsoft 365 through Microsoft Graph. It is separate from File Finder so the same authenticated connection can support additional Project Notes features in the future.

The **Microsoft 365 drafts** integration creates email drafts and draft calendar events in your mailbox. File Finder discovers project channels in Microsoft Teams and the documents in their SharePoint-backed folders.

Native [Contact Synchronization](Preferences.md#contact-synchronization) and **Full Contact Export** also use this account. Use this tab's single sign-in action to grant `Contacts.ReadWrite` alongside email and calendar access. Contact export remains available when scheduled and manual synchronization are disabled.

## For Administrators

Project Notes signs in to Microsoft 365 as the user, through an app registration your organization owns. This section lists everything a Microsoft Entra administrator sets up so every Office 365 feature works. Users need nothing beyond the tenant ID and application (client) ID it produces.

### 1. Create a dedicated app registration

In the Microsoft Entra admin center, open **App registrations > New registration**.

- **Supported account types** must agree with the tenant ID users enter in Project Notes. The default tenant, `organizations`, needs a registration that accepts accounts in any organizational directory. A registration limited to your own directory works when users enter your specific tenant ID.
- **Redirect URI** stays empty, and no client secret or certificate is needed.
- On the **Authentication** page, set **Allow public client flows** to **Yes**. Project Notes signs in with the device-code flow, and without this setting sign-in fails after the user enters the code.

Do not reuse the registration made for the old Outlook Integration plugin. Its consent covers only Mail, Calendar, Contacts, and Tasks.

### 2. Add the delegated Microsoft Graph permissions

On **API permissions**, add these **delegated** Microsoft Graph permissions. Project Notes uses no application permissions, so it can reach only what the signed-in user can already reach.

| Permission | Used for |
| :--- | :--- |
| `User.Read` | Identifying the signed-in account |
| `offline_access` | Staying signed in between sessions |
| `Mail.ReadWrite` | Creating email drafts, and reading mail for [Archive Mail](../StandardPlugins/ArchiveMail.md) |
| `Calendars.ReadWrite` | Creating draft meeting events |
| `Contacts.ReadWrite` | [Contact Synchronization](Preferences.md#contact-synchronization) and **Full Contact Export** |
| `Team.ReadBasic.All` | File Finder listing the teams the user has joined |
| `Channel.ReadBasic.All` | File Finder listing each team's channels |
| `Files.ReadWrite.All` | File Finder reading channel files, and saving reports, documents made from templates and archived email into a project's Teams folder |

Notes on the list:

- **Never `Mail.Send`.** Project Notes cannot send mail from the account. `Mail.ReadWrite` also covers plugins that ask for `Mail.Read`, such as Archive Mail.
- **No SharePoint permissions.** Teams keeps channel files in SharePoint, but `Files.ReadWrite.All` reaches them. `Sites.Read.All` and `Sites.ReadWrite.All` are not needed.
- **Read-only Teams access.** If your organization will not allow `Files.ReadWrite.All`, add `Files.Read.All` instead and have users turn off **Save files into Microsoft Teams folders** (see [Settings](#settings)). File Finder still reads Teams, and saves go to the project's local folder. A project whose only folder is in Teams then gets no **Save to Project Folder** button, and plugins ask where to save. Earlier versions asked only for `Files.Read.All`; users who signed in then must sign in again once `Files.ReadWrite.All` is granted.
- **Read-only contacts.** Importing contacts without ever exporting them needs only `Contacts.Read`, but Project Notes asks for `Contacts.ReadWrite` whenever Office 365 is the contact provider. Grant `Contacts.ReadWrite` unless you know nobody will export.
- **Plugins.** A plugin can declare more Microsoft Graph permissions in its `plugin.json` (`office365.scopes`). The bundled plugins need nothing beyond the table. Before users install a third-party plugin that asks for more, add its permissions to the registration too.

### 3. Grant admin consent

Select **Grant admin consent for** your organization on the **API permissions** page, covering every permission above.

Microsoft lets users consent to all of these delegated permissions themselves. When your tenant allows that, each user approves what they use at sign-in, and admin consent is optional. Many tenants block user consent, though. Then every permission Project Notes asks for needs admin consent, or Microsoft refuses the whole sign-in, and email drafts, calendar events and contacts stop working along with Teams.

The sign-in under **Settings > Office 365 Integration** asks for mail and calendar access, contact access when Office 365 is the contact provider, and the three Teams permissions when File Finder searches Teams or the account already uses Teams files. Granting admin consent for the whole list up front means turning on a feature later never needs a new approval.

A sign-in that seems to complete but leaves File Finder reporting an error that mentions admin approval (AADSTS90094) means consent is missing. Either it was never granted, or the tenant ID or client ID entered in Project Notes points to a different registration.

### 4. Allow the device-code sign-in

Conditional Access policies that block the device-code flow (the **Authentication flows** condition) also block Project Notes. Exclude this app registration from such a policy, or limit the exclusion to the users who need it.

### 5. Check what users can reach in Teams

Delegated permissions never go beyond the user's own access:

- File Finder sees only teams the user has joined, and private or shared channels only when the user is a member of them.
- Saving into a Teams folder needs edit rights to the channel's files. Team members have them by default. A user with read-only access to the channel's SharePoint folder can browse it in File Finder, but saves there are refused.

## Settings

Open the app menu, choose **Settings**, and select **Office 365 Integration**.

| Setting or action | Description |
| :--- | :--- |
| **Microsoft Entra tenant ID** | The directory (tenant) ID for the organization that owns the app registration. The default `organizations` allows sign-in with a work or school account from any Microsoft Entra organization. Use a specific tenant ID when access should be restricted to one organization. |
| **Application (client) ID** | The application ID assigned to the Microsoft Entra app registration. |
| **Sign in to Microsoft 365** | Begins device-code sign-in and requests email, calendar, and contact permissions. Project Notes displays a code and a button that opens Microsoft's sign-in page. Copy the code, open the page, and complete sign-in in your browser. |
| **Sign out** | Removes the saved refresh token and invalidates cached Office 365 File Finder state. It does not remove local File Finder settings or existing **Files & Folders** entries. |
| **Save files into Microsoft Teams folders** | On by default. Lets saved reports, documents made from templates and archived email go into a project's Teams folder, which needs `Files.ReadWrite.All`. Turn it off when your organization does not approve that permission, then sign in again. |
| **Sign in without saving to Teams** | Shown after a sign-in that asked for `Files.ReadWrite.All` fails. It turns off **Save files into Microsoft Teams folders** and signs in again. |

The status panel reports whether Project Notes is signed in and shows the device code and verification-page button while authentication is in progress. When saving into Teams is on but Microsoft granted read access only, the note under the setting says so, and Teams folders stay read-only until an administrator grants `Files.ReadWrite.All` and you sign in again.

## Credential Storage

Project Notes stores the Microsoft refresh token in the operating system's credential vault. Access tokens remain in memory. The tenant ID, client ID, and connection state are stored in the local Project Notes settings profile and do not sync through the project database. These settings are kept independent of the old Outlook Integration plugin's tenant ID and application ID. Changing one never affects the other.

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

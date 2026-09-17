# Cross-platform email investigation

Date: 2026-09-12. Branch: `feature/email`. Status: recommendation; no implementation or live mail-client validation yet.

Follow-up: the selected five-backend desktop scope and delivery sequence are in the [Native QML email implementation plan](NativeEmailImplementationPlan.md). That plan supersedes the broader provider/platform suggestions below for this feature.

## Recommendation

Provide a small ProjectNotes email preparation dialog backed by interchangeable delivery adapters. Default to preparing a message in a supported, configured mail application. Offer Microsoft 365 and, subsequently, Gmail connections for users who need consistent HTML and attachment handling across operating systems or use webmail exclusively.

The easiest experience for someone with an existing mail setup is to review and send in that application. The most predictable implementation for supported cloud accounts is an official provider API. Combining these paths gives better coverage than selecting either as the only solution. This is an architectural judgment based on the documented capabilities below, not a claim that all clients have been tested.

Use a PDF attachment plus a short editable message as the default for reports. Offer inline HTML where the selected adapter supports it. If inline HTML with recipients and attachments must work identically on every platform, prioritize provider APIs and a ProjectNotes composer instead of making native sharing the default.

## Existing foundations

- `plugins/includes/collaboration_tools.py:170`: `send_an_email` already assembles recipients, expands templates, gathers attachments, and chooses the Outlook COM or Graph path. This is the natural migration point for desktop plugins.
- `plugins/includes/outlook_tools.py:480`: `send_email` creates and displays an Outlook COM message, adds the existing signature, and attaches files. It does not call Send.
- `plugins/includes/graphapi_tools.py:538`: `draft_an_email` creates a Microsoft Graph draft and uploads attachments. It currently discards the draft identifier on success and does not open the draft for review.
- `plugins/exportnotes_plugin.py:439`: the PDF completion callback invokes the common email path for inline HTML, HTML attachments, or PDF attachments. Report plugins use the same collaboration class.
- `plugins/includes/graphapi_tools.py:29`: the older plugin authentication requests mail, contacts, calendar, and task scopes together and serializes a token cache to a temporary-folder file.
- `ProjectNotesFileFinder/MicrosoftOAuthManager.cpp` and `FileFinderService.cpp:85`: newer native Microsoft authentication already supports refresh and credential-store callbacks. The documented Office 365 connection currently serves File Finder and has separate settings and scopes from plugin email. Generalize this authentication infrastructure before creating another independent sign-in system; adding mail permissions still needs consent.

## Options

| Approach | User experience | Main limitation | Role |
| --- | --- | --- | --- |
| Qt `mailto:` | Opens the configured mail handler with a short message; little setup | No portable attachment or HTML-body contract; client URL limits | Basic-message fallback |
| Native compose/share APIs | User selects an existing account/app and reviews before sending | Different capabilities per OS and receiving app | Default when verified for the requested content |
| Per-app automation: Outlook COM, AppleScript, client command lines | Can integrate closely with a particular client | Requires individual client support and maintenance | Keep existing classic Outlook support; avoid making this the foundation |
| ProjectNotes composer + Graph/Gmail | Consistent recipients, HTML, attachments, and explicit Send across OSes | Provider-specific OAuth setup, consent, and account restrictions | Optional connected-account experience |
| ProjectNotes composer + SMTP | Broad protocol coverage and control of the message | Server/authentication configuration; signatures, drafts and Sent-folder behavior become our responsibility | Later advanced option if users need other providers |
| Export `.eml` | Portable MIME message content | Opening as an editable unsent draft is client-dependent | Optional export, subject to client testing |

Qt documents `mailto:` support but warns about long URLs and explains that a successful launch request does not prove that the external app opened it. RFC 6068 defines the body as plain text intended for short messages, rather than arbitrary MIME content. Attachment query parameters are client extensions, not a portable baseline. [Qt QDesktopServices](https://doc.qt.io/qt-6/qdesktopservices.html), [RFC 6068](https://datatracker.ietf.org/doc/html/rfc6068).

Python's standard library includes an SMTP client, so basic transmission needs no new desktop dependency. That does not provide account discovery, provider OAuth onboarding, a composer, or mailbox draft synchronization. Do not assume the same Python backend can be used by the separately built iOS app. [Python smtplib](https://docs.python.org/3/library/smtplib.html).

## Native platform candidates

| Platform | Candidate API | Constraint to validate |
| --- | --- | --- |
| Windows | Windows Share through `DataTransferManager` / `IDataTransferManagerInterop`, with files in `DataPackage`; retain classic Outlook COM | Receiving-app support and field preservation vary. Test new Outlook separately; sharing is not a universal To/CC/BCC/HTML email-composition contract. |
| macOS | `NSSharingService` with `composeEmail`, or a sharing picker | Discover availability; verify actual mail-app choice, attachments, recipients and rich text. Do not assume it controls every default mail client. |
| Linux | D-Bus `org.freedesktop.portal.Email.ComposeEmail` | Portal backend and configured client must support the attachment handoff; portal presence alone is insufficient. |
| iOS/iPadOS | `MFMailComposeViewController` when `canSendMail()` succeeds; `UIActivityViewController` for other sharing targets | Built-in Mail composition depends on a configured Mail account. Other apps use their own share extensions, whose accepted fields vary. |

Microsoft explicitly lists the Outlook Object Model and MAPI as unsupported in new Outlook. Desktop Windows apps can invoke the Share UI through window-handle interop and supply file items. Microsoft's comparison also links to the newer `TransferTarget` API; the linked reference includes prerelease caveats, so investigate runtime/OS availability before relying on it. [Outlook comparison](https://support.microsoft.com/en-us/outlook/getstarted/feature-comparison-between-new-outlook-and-classic-outlook), [desktop Share interop](https://learn.microsoft.com/en-us/windows/apps/develop/ui/display-ui-objects), [file sharing](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.datatransfer.datapackage.setstorageitems), [TransferTarget](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.datatransfer.transfertarget).

Apple provides separate desktop and mobile APIs. The mobile mail composer supports message editing and attachments; its Mail outbox handoff is not proof of recipient delivery. Use the share sheet when the user wants another installed sharing application. [macOS composeEmail](https://developer.apple.com/documentation/appkit/nssharingservice/name/composeemail), [iOS mail composer](https://developer.apple.com/documentation/messageui/mfmailcomposeviewcontroller), [mail availability](https://developer.apple.com/documentation/messageui/mfmailcomposeviewcontroller/cansendmail()), [iOS sharing](https://developer.apple.com/documentation/uikit/collaborating-and-sharing-copies-of-your-data).

The Linux Email portal accepts recipients, subject, body and attachment file descriptors. Its documentation also requires the host email client to understand attachment URI parameters. HTML-body fidelity is not promised by this interface. [Email portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Email.html).

## Connected accounts

For Microsoft mailboxes, extend the existing Graph integration. Draft creation supports HTML and attachments and returns a message object; delegated draft access requires `Mail.ReadWrite`. Preserve the returned ID, complete attachments, then offer to open the returned `webLink` if available, with an explicit Drafts-folder fallback. Verify draft editing and account selection in the browser before promising one-click review. Graph connects to the mailbox provider, so a Gmail account merely displayed inside Outlook is not thereby covered. [Graph draft creation](https://learn.microsoft.com/en-us/graph/api/user-post-messages?view=graph-rest-1.0), [Graph mail overview](https://learn.microsoft.com/en-us/graph/outlook-mail-concept-overview).

For direct sending from ProjectNotes, Graph supports `Mail.Send`; its `202 Accepted` response indicates acceptance, not completed delivery. Preserve a clear distinction between opening a composer, creating a draft, submission, and delivery. [Graph sendMail](https://learn.microsoft.com/en-us/graph/api/user-sendmail?view=graph-rest-1.0).

Gmail supports MIME drafts and sending. A meaningful product tradeoff is that `gmail.send` is sensitive, while `gmail.compose`, needed for draft management, is restricted. Public-app verification requirements differ; restricted scopes can also require a security assessment when restricted data is stored on or transmitted through servers. Assess the actual architecture and applicable exemptions before selecting Gmail draft-first behavior. A local ProjectNotes composer with send-only authorization deserves preference if it satisfies the workflow. [Gmail drafts](https://developers.google.com/workspace/gmail/api/guides/drafts), [Gmail scopes](https://developers.google.com/workspace/gmail/api/auth/scopes).

For a convenient public release, aim for maintained application registrations and normal provider sign-in, with organization-specific registrations as an advanced option. The feasibility of that distribution model and organizational consent policies needs validation. Keep authentication on the provider's supported sign-in surface and store reusable credentials in OS credential storage. Provider-created messages must not be assumed to inherit client signatures or all client-side send policies; validate those requirements with intended users.

## Proposed application contract and UX

Keep content preparation independent of the transport: an `EmailRequest` contains To/CC/BCC, subject, plain text, optional HTML, and attachment metadata/paths. An `EmailService` exposes availability and capabilities and separate compose, create-draft and send operations. Adapters report unsupported fields rather than silently dropping them. These names describe a proposed interface, not existing classes.

The dialog should prefill recipients using the existing project/meeting selection rules, show the selected account or app, list actual attachments, and allow a short message edit. Remember the user's route. Avoid two full composition steps: native handoff needs a lightweight preparation view; a provider-backed Send action needs the complete review experience inside ProjectNotes.

Return explicit states such as composer requested, draft created, submitted, cancelled, failed, or outcome unknown. Maintain attachment files until the receiving application no longer needs them. On a missing app, unavailable account, or unsupported attachment handoff, retain the prepared content and offer Save PDF and Copy message/recipients. Never convert an ambiguous submission timeout into an automatic resend through another adapter.

Expose the shared contract through the desktop controller and existing Python bridge. Keep native UI adapters outside the shared database layer. If mobile is included, share the C++ contract where practical and wire the mobile adapter into its separate build; the desktop Python plugin workflow is not automatically available on iOS.

## Suggested implementation order

1. Extract the message/service contract around current email preparation; retain classic Outlook and Graph behavior. Separate email settings from calendar/contact synchronization choices.
2. Improve Graph draft results and review flow, using the newer native authentication/credential infrastructure where practical. Handle attachment failures and partial drafts explicitly.
3. Prototype native handoff for the supported desktop platforms with one real PDF and a short message. Ship only combinations that pass the compatibility checks; label sharing limitations accurately.
4. Add a small QML preparation dialog and export/copy fallback. Use PDF by default; enable inline HTML only for capable routes.
5. Add Gmail if user demand justifies account integration and verification. Choose provider draft review versus local compose/send based on permission and UX tradeoffs. Add SMTP only for a demonstrated provider gap.
6. Add and verify iOS integration separately if included in this feature's release scope.

Before implementation, the largest unresolved product question is whether initial users predominantly use Microsoft 365, Gmail, or diverse local clients. Microsoft 365-heavy usage favors improving Graph first; a diverse audience favors native PDF sharing first. No usage distribution was assumed in this investigation.

## Verification required before shipping

Exercise classic and new Outlook on Windows, Apple Mail and an alternative macOS client, and GNOME/KDE with representative Linux clients. If mobile is included, test a real iPhone/iPad with Mail configured and with only a third-party mail app. For connected accounts, test personal/work account support as intended, expired consent, and organizational restrictions.

Check recipient selection and To/CC/BCC preservation; Unicode names/subjects/filenames; HTML tables; PDF contents; multiple/missing/large attachments; signatures and account choice; no configured handler; cancel; offline/authentication failure; partial attachment uploads; ambiguous network outcomes; and repeated clicks. Mock meaningful adapter failure cases and perform live client handoff checks using maintainer-controlled accounts. No messages were sent and no runtime compatibility checks were performed for this investigation.

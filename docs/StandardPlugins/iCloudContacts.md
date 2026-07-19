# iCloud Contacts

Project Notes can import contacts from every address book in iCloud. The sync runs when Project Notes starts, every five minutes while it is open, or on demand from **Plugins > Utilities > Sync iCloud Contacts Now**.

iCloud supplies the contact name, preferred email, work and mobile phone numbers, company/client, and job title/role. Changes in iCloud overwrite those fields in Project Notes. Removing a contact from iCloud never removes the person or their project relationships from Project Notes.

Optionally, enable **Export new Project Notes contacts not found in iCloud**. Project Notes compares trimmed contact names across every iCloud address book and creates only missing contacts; it never overwrites an existing iCloud contact. New contacts are added to the address book named **Contacts**, or the first available address book when no book has that name.

## Setup

1. Protect your Apple Account with two-factor authentication.
2. Sign in at [account.apple.com](https://account.apple.com/), open **Sign-In and Security > App-Specific Passwords**, and create a password for Project Notes. Apple provides [app-specific password instructions](https://support.apple.com/102654).
3. In Project Notes, choose **Plugins > Settings > iCloud Contacts**.
4. Enter the email address used by your Apple Account and the app-specific password.
5. Select **Test Connection**. Project Notes reports how many iCloud address books it found.
6. To send new Project Notes contacts to iCloud, select **Export new Project Notes contacts not found in iCloud**.
7. Enable scheduled imports and save.

The app-specific password is stored in macOS Keychain, Windows Credential Manager, or the Linux desktop Secret Service. It is never saved in the Project Notes plugin settings or database. Changing or resetting the main Apple Account password revokes app-specific passwords; create and save a replacement if imports begin reporting an authentication failure.

## Matching and conflicts

Project Notes requires active people to have unique names. On the first import, an iCloud contact with the exact same trimmed name is linked to that existing person. Later imports retain that link, including when the contact is renamed in iCloud.

Contacts without a display name are skipped. If multiple iCloud contacts have the same name, or an iCloud rename conflicts with another PN person, the importer skips the conflicting contacts and reports the count rather than merging unrelated people.

Photos, postal addresses, notes, groups, and list membership are not imported. For a manual alternative, export one or more contacts as a `.vcf` file from iCloud.com and drop the file onto the Project Notes People list.

## Troubleshooting

- **Authentication failed:** Verify the Apple Account email and generate a new app-specific password. The regular Apple Account password cannot be used.
- **Secure storage unavailable:** Unlock or configure the operating-system credential store. Scheduled imports remain disabled rather than storing the password as plain text.
- **Network or CardDAV error:** Use **Test Connection**. Transient errors are retried on the next five-minute interval.
- **Contact skipped:** Check for a missing name, a duplicate iCloud name, or a conflicting PN person name.

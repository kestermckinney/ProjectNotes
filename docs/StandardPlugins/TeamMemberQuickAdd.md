# Team Member Quick Add

The **Team Member Quick Add** plugin provides a quick dialog for adding new contacts to a project team without navigating to the Contacts List Page first. It works on all platforms.

## How It Works

When invoked from a project, the plugin opens a small dialog where you can enter the new team member's information. The plugin will:

1. Create a new **Contact** record in the contacts list (if the contact doesn't already exist).
2. Create or look up the **Company** record.
3. Add the contact to the **Project Team** with the role you specify.

## Using Team Member Quick Add

**To add a new team member to a project:**

1. Right-click on the project in the **Project List Page**.
2. Choose **Team Member Quick Add**.
3. Fill in the team member details in the dialog:

| Field | Description |
| :--- | :--- |
| **Name** | Full name of the team member. |
| **Email** | Email address. |
| **Company** | The company the contact belongs to. |
| **Role** | The role this contact plays on the project. |
| **Phone** | Phone number (optional). |

4. Click **OK** to add the contact.

The new team member will immediately appear in the project's **Team** tab and in the **Contacts List Page**.

## Notes

- If a contact with the same name already exists in the database, the plugin will use the existing record rather than creating a duplicate.
- The company name must match an existing company or a new company will be created automatically.
- The project manager specified in [Preferences](<../InterfaceOverview/Preferences.md>) is available as a team member without needing to be added through this dialog.

# Team Member Quick Add

The **Team Member Quick Add** plugin provides a quick dialog for adding new people to a project team without navigating to the People List Page first. It works on all platforms.

## How It Works

When invoked from a project, the plugin opens a small dialog where you can enter the new team member's information. The plugin will:

1. Create a new **Person** record in the people list (if the person doesn't already exist).
2. Create or look up the **Client** (company) record.
3. Add the person to the **Project Team** with the role you specify.

## Using Team Member Quick Add

**To add a new team member to a project:**

1. Right-click on the project in the **Project List Page**.
2. Choose **Team Member Quick Add**.
3. Fill in the team member details in the dialog:

| Field | Description |
| :--- | :--- |
| **Name** | Full name of the team member. |
| **Email** | Email address. |
| **Company** | The company (client) the person belongs to. |
| **Role** | The role this person plays on the project. |
| **Phone** | Phone number (optional). |

4. Click **OK** to add the person.

The new team member will immediately appear in the project's **Team** tab and in the **People List Page**.

## Notes

- If a person with the same name already exists in the database, the plugin will use the existing record rather than creating a duplicate.
- The company name must match an existing client or a new client will be created automatically.
- The project manager specified in [Preferences](<../InterfaceOverview/Preferences.md>) is available as a team member without needing to be added through this dialog.

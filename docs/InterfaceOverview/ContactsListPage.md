# Contacts List Page

## Managing Your List Of Contacts

Project Notes keeps a list of contacts that can be associated with projects. Native [contact synchronization](Preferences.md#contact-synchronization) imports from Outlook (classic), Microsoft 365, Thunderbird, or iCloud according to your integration choice. You can select source address books and review uncertain contact or company matches. Project Notes may have a customization that imports and exports names from other enterprise applications such as an accounting system or customer relationship manager. Native matching uses saved source identities and asks you to review uncertain matches before creating a contact.

## The Contacts List Page Layout

To navigate to the Contacts List Page, click the **Contacts** icon in the icon rail. The view of contacts can be filtered using the [Filter Tool](<FilterTool.md>), or narrowed with the quick-search box above the list.

**To create a new contact:**

1. Click **+ New** in the bar above the list, or choose **New Record** from the app menu (the menu icon at the top of the icon rail).
2. Type in the **Name**. **Name** is a required field.
3. Type in the **Email**, **Office Phone**, **Cell Phone** if available. A contact without an **Email** cannot be picked as a report recipient or a meeting attendee.
4. Type in the **Role**. **Role** will be copied to the project team member role by default when a contact is added to a project.
5. Select the **Company** from the drop down.



**To delete a contact:**

1. Right-click the contact's card (or click its **⋮** button) to open its menu.
2. Choose **Delete**.
3. Click **Yes** to confirm the deletion. Note: You may not be able to delete contacts if they are associated with other project information. In this case, clicking **Yes** opens the Search Page — see [Search Page](<SearchPage.md>) for how to work with search results.

**To navigate to a contact's company:**

1. Right-click the contact's card (or click its **⋮** button) to open its menu.
2. If the contact has a **Company** assigned, choose **Go To Company** from the menu.
3. The application will automatically switch to the Companies page and open that company's record.

**To share a contact card (vCard):**

1. Right-click the contact's card (or click its **⋮** button). Team members on a project's **Team** tab and attendees on a meeting note offer the same choices for the person behind the row.
2. Choose **Email Contact…** and a draft opens right away in your configured email integration, with the contact's `.vcf` file attached. The draft has no recipients and the subject `Contact: <name>`. Add the recipients in your email client.
3. Or choose **Export Contact…** to save the `.vcf` file. Outlook, Apple Contacts, Thunderbird and phones can import it.

You can also drag the contact's card out of Project Notes. Drop it on a folder or the desktop to save `<name>.vcf`, or on a mail message or contacts app to hand over the card.

The card holds the name, company, role, email, office phone and cell phone. Fields left blank are left out.

**To import contacts from vCards:**

Drop one or more vCards on the contacts list: a `.vcf` file, or a contact dragged from Outlook or Apple Contacts. Each contact's company is matched to an existing company or created. A contact whose name is already in the list isn't added again.

**To edit a contact:**

1. Click the contact's card to open it.
2. Type in or select the new value in any field — changes save automatically.
3. On the contact detail page, click the **⋮** button in the top-right to access additional actions:
   - **Export XML** — Export the contact's record
   - **Go To Company** — Navigate to the contact's assigned company (if assigned)
   - **Refresh** — Reload the contact's data from the database
   - **Plugins** — Any plugin actions configured for contact records

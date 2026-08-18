# Presenting To Clients

When sharing your screen with clients, you want to control what information they see. Project Notes provides tools to help you present a clean view that shows only project information relevant to the specific client you're meeting with, while hiding:

- Internal notes and comments meant only for your team
- Sensitive tracker items or risks
- Information about other clients
- Financial metrics (Earned Value, budgets, actual costs) that you don't want to discuss

This document explains how to use the **Show Internal / Budget Items** toggle and the Filter Tool together to create a client-appropriate view. Both toggles and the Filter Tool are reached from the app menu — the menu icon at the top of the icon rail.

## Two-Part Strategy: Internal Items + Filtering

The most effective approach uses both tools together:

1. **Show Internal / Budget Items** — Hide or show internal notes, risks, and sensitive information
2. **Filter Tool** — Show only data relevant to the specific client

### Part 1: Marking Sensitive Content as Internal

Some notes and tracker items contain sensitive information meant only for your internal team. You can mark these as "internal" so they disappear with a single toggle when presenting to clients.

**Items that can be marked as internal:**

- **Meeting Notes** — Sensitive discussion points, internal observations, or team-only notes
- **Tracker Items** — Internal risks, issues, or action items that are not relevant to the client
- **Earned Value Metrics** — Budget information and cost metrics

**To mark a meeting note as internal:**

1. Open the meeting note from the [Project Page](<ProjectPage.md>)'s **Notes** tab.
2. Check the **Internal item** checkbox.
3. The note will be hidden from the list when **Show Internal / Budget Items** is turned off.



**To mark a tracker item as internal:**

1. Open the tracker item from the [Item Tracker Page](<ItemTrackerPage.md>) or [Action Item Detail Page](<ActionItemDetailPage.md>).
2. Check the **Internal item** checkbox.
3. The item will be hidden from the list when **Show Internal / Budget Items** is turned off.



**To hide all internal content before presenting:**

1. From the app menu, uncheck **Show Internal / Budget Items**.
2. All notes and tracker items marked as internal are now hidden from the list.
3. Financial metrics (budget, actual costs, earned value) are also hidden from the Project List and Project Page.

Note: unlike the note or item lists, the **Internal item** checkbox on an open note or item's own page stays visible either way — it's the note/item's presence in the list, and the financial figures, that this toggle controls, not the checkbox itself.

**To show internal content again after presenting:**

1. From the app menu, check **Show Internal / Budget Items**.

### Part 2: Using the Filter Tool to Show Only a Specific Client's Data

Even with internal items hidden, your screen might still show projects and information from other clients. Use the Filter Tool to show only the current client's data.

**Common filtering scenarios:**

- **Show only one client's projects** — Filter the Projects list by the Client field
- **Show only one project's notes or tracker items** — Rather than filtering, simply open that project and use its **Notes** or **Tracker** tab, which already shows only that project's data

**To filter projects by client:**

1. From the Project List Page, click **Filter** in the bar above the list (or choose **Filter Data…** from the app menu).
2. In the Filter Editor, select **Client** from the column list on the left.
3. In the values panel, click the client you want to show (for example, "Acme Corporation").
4. Click **Apply**.
5. The Projects list now shows only projects for the selected client.

## Setting Up Your View Before a Client Meeting

Here's a step-by-step workflow to prepare your view before sharing your screen with a client:

**Before the meeting:**

1. Open the Project List Page, or navigate to the specific project you'll be discussing.
2. Decide which information the client should see.
3. Use the Filter Tool to show only that client's projects, or open the specific project directly.
4. Review the visible data to ensure nothing sensitive is showing.

**When the client joins the call:**

1. From the app menu, uncheck **Show Internal / Budget Items** to hide internal notes, risks, and financial metrics.
2. Your screen now shows only client-appropriate information.
3. Share your screen with the client.

**After the meeting:**

1. From the app menu, check **Show Internal / Budget Items** to restore the full view.
2. Clear any active filter by opening the Filter Editor and clicking **Reset all**, then **Apply**.

## Important Notes

- **Filters are session-only** — Filters you set in the Filter Editor apply immediately but are not saved to the database; they clear the next time you restart Project Notes. Remember to clear filters after presenting to clients if you plan to leave the app running.
- **Internal items are hidden from lists** — When **Show Internal / Budget Items** is turned off, internal notes and tracker items disappear from their lists, and financial information (budget, actual costs, earned value) disappears from the Project List and Project Page.
- **Test your view first** — Before sharing your screen, review what's visible to ensure no sensitive information is showing.

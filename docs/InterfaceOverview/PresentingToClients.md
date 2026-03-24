# Presenting To Clients

When sharing your screen with clients, you want to control what information they see. Project Notes provides tools to help you present a clean view that shows only project information relevant to the specific client you're meeting with, while hiding:

- Internal notes and comments meant only for your team
- Sensitive tracker items or risks
- Information about other clients
- Financial metrics (Earned Value, budgets, actual costs) that you don't want to discuss

This document explains how to use the Internal Items feature and the Filter Tool together to create a client-appropriate view. For more details about the **View** menu options, see [View Menu](<ViewMenu.md>).

## Two-Part Strategy: Internal Items + Filtering

The most effective approach uses both tools together:

1. **Internal Items** — Hide or show internal notes, risks, and sensitive information
2. **Filter Tool** — Show only data relevant to the specific client

### Part 1: Marking Sensitive Content as Internal

Some notes and tracker items contain sensitive information meant only for your internal team. You can mark these as "internal" so they disappear with a single menu click when presenting to clients.

**Items that can be marked as internal:**

- **Meeting Notes** — Sensitive discussion points, internal observations, or team-only notes
- **Tracker Items** — Internal risks, issues, or action items that are not relevant to the client
- **Earned Value Metrics** — Budget information and cost metrics

**To mark a meeting note as internal:**

1. Open the meeting note in the [Project Notes Page](<../InterfaceOverview/NotesPage.md>).
2. Check the **Internal Note** checkbox.
3. The note will be hidden when "Show Internal Items" is unchecked in the View menu.

**To mark a tracker item as internal:**

1. Open the tracker item in the [Item Tracker Page](<ItemTrackerPage.md>) or [Action Item Detail Page](<ActionItemDetailPage.md>).
2. Check the **Internal** checkbox.
3. The item will be hidden when "Show Internal Items" is unchecked in the View menu.

**To hide all internal content before presenting:**

1. From the **View** menu, uncheck **Internal Items**.
2. All notes and tracker items marked as internal are now hidden from the screen.
3. The Internal checkbox itself is also hidden, so clients cannot see that some items are marked internal.
4. Financial metrics (budget, actual costs, earned value) are also hidden.

**To show internal content again after presenting:**

1. From the **View** menu, check **Internal Items**.

### Part 2: Using the Filter Tool to Show Only a Specific Client's Data

Even with internal items hidden, your screen might still show projects and information from other clients. Use the Filter Tool to show only the current client's data.

**Common filtering scenarios:**

- **Show only one client's projects** — Filter the Projects list by the Client column
- **Show only one project's notes** — Filter the Project Notes list by the Project column
- **Show only one project's tracker items** — Filter the Item Tracker by the Project column

**To filter projects by client:**

1. From the Projects List Page, open the [Filter Tool](<FilterTool.md>) by selecting **Filter** from the **View** menu.
2. In the Filter Tool window, select **Client** from the Column Name list.
3. In the Filter Values list, click the checkbox next to the client you want to show (for example, "Acme Corporation").
4. Click **Apply**.
5. The Projects list now shows only projects for the selected client.

**To filter meeting notes by project:**

1. From a Project Details page, navigate to the Notes tab.
2. Open the Filter Tool by selecting **Filter** from the **View** menu.
3. In the Filter Tool window, select **Project Name** from the Column Name list.
4. In the Filter Values list, click the checkbox next to the project you want to show.
5. Click **Apply**.
6. The Notes list now shows only notes for that specific project.

**To filter tracker items by project:**

1. From a Project Details page, navigate to the Tracker tab.
2. Open the Filter Tool by selecting **Filter** from the **View** menu.
3. In the Filter Tool window, select **Project** from the Column Name list.
4. In the Filter Values list, click the checkbox next to the project you want to show.
5. Click **Apply**.
6. The Tracker Items list now shows only items for that specific project.

## Setting Up Your View Before a Client Meeting

Here's a step-by-step workflow to prepare your view before sharing your screen with a client:

**Before the meeting:**

1. Open the Projects List Page or navigate to the specific project.
2. Decide which information the client should see.
3. Use the Filter Tool to show only that client's projects or the relevant project's data.
4. Review the visible data to ensure nothing sensitive is showing.

**When the client joins the call:**

1. From the **View** menu, uncheck **Internal Items** to hide all internal notes, risks, and financial metrics.
2. Your screen now shows only client-appropriate information.
3. Share your screen with the client.

**After the meeting:**

1. From the **View** menu, check **Internal Items** to restore the full view.
2. Clear any active filters by opening the Filter Tool and clicking **Reset**, then **Apply** to see all data again.

## Important Notes

- **Filters are persistent** — Filters you set are saved in the database and remain active when you close and reopen Project Notes. Remember to clear filters after presenting to clients.
- **Internal items are hidden completely** — When "Show Internal Items" is unchecked, clients cannot see that any items are hidden. The Internal checkbox itself disappears from forms.
- **Earned Value metrics are context-sensitive** — When "Show Internal Items" is unchecked, financial information (budgets, actual costs, earned value) does not appear in the Projects list or Project Details page.
- **Test your view first** — Before sharing your screen, review what's visible to ensure no sensitive information is showing.
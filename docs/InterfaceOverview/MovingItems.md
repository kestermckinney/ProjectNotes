# Moving Items

Moving a Tracker/Action Item reassigns it to a different project and, optionally, a different meeting note — unlike [Copying Items](<CopyingItems.md>), which leaves the original in place and creates a duplicate. There are two ways to move an item: the **Move To…** menu, and dragging it onto a project in the sidebar.

## Using Move To…

**To move an item with Move To…:**

1. From the master **Items** list, the **Tracker Items** list on a project's own page, or the [Action Item Detail Page](<ActionItemDetailPage.md>), right-click the item (or click its kebab **⋮** button).
2. Choose **Move To…**.
3. Choose the destination **Project**. Choosing a different project resets the **Meeting** choice, since meetings are specific to one project.
4. Choose a **Meeting** to link the item to, or leave it as **No meeting**.
5. Review any renumber or team-membership notes shown below the fields (see below), then click **Move**.

Nothing changes until you click **Move** — closing the dialog any other way leaves the item where it was.

## Dragging an Item onto a Project

**To move an item by dragging:**

1. In the project sidebar, drag the item's row onto the project you want to move it to.
2. If the move needs your attention (see below), a confirmation dialog appears — review it and click **Move**. Otherwise the move happens immediately.

Dragging has no meeting-picker step: if the item is linked to a meeting, that link is simply cleared, since meetings belong to a single project and can't follow the item to a new one. Use **Move To…** instead if you want to link the item to a meeting on the destination project.

## What Can Change When You Move an Item

Moving an item to a different project can trigger two side effects, both shown to you before the move completes:

- **Renumbering** — Item numbers restart at 0001 within each project, so if the item's current number is already taken in the destination project, it's assigned the next available number there. The dialog shows both the old and new numbers.
- **Team membership** — If the item's **Assigned To** or **Identified By** person is not yet on the destination project's team, they are added to it automatically as part of the move. The dialog lists anyone who will be added.

If neither applies, the move happens immediately without a confirmation step.

## Related Documentation

- [Copying Items](<CopyingItems.md>) — duplicate an item instead of moving it
- [Item Tracker Page](<ItemTrackerPage.md>) — the master list items are moved from
- [Action/Tracker Item Details](<ActionItemDetailPage.md>) — open an item to move it from its own page
- [Project Folders](<ProjectFolders.md>) — dragging rows in the sidebar for a different purpose (organizing projects, not items)

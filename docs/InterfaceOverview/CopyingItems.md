# Copying Items

The Copy Item action lets you duplicate a risk, issue, or action item without re-typing its details — useful when you need several similar tracker items on the same project. To reassign an existing item to a different project or meeting instead of duplicating it, see [Moving Items](<MovingItems.md>).

**To copy an item:**

1. From the master **Items** list, or from the **Tracker Items** list on a project's own page, right-click the item you want to duplicate (or click its kebab (**⋮**) button).
2. Choose **Duplicate** from the menu.

A new item is created immediately in the same project, and it opens automatically so you can adjust it.

## What Gets Copied

The duplicate starts from the original item's fields:

- The item type, description, priority, status, assigned-to, identified-by, and due/identified dates are copied as-is.
- The new item is given the next available item number for that project.
- Its name is prefixed with **"Copy of"** so it's easy to tell apart from the original in the list.
- The item is assigned its own internal ID, entirely independent of the original.

Update history and progress notes attached to the original item are not carried over — the copy starts with a clean history, and later changes made to either item do not affect the other.

## Duplicating from Other Views

Choosing **Duplicate** always creates the copy in that same item's project, regardless of whether you opened the menu from the master **Items** list (which spans every project) or from the **Tracker Items** list on the project's own page.

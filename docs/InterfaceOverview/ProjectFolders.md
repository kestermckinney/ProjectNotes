# Project Folders

Project Folders group projects in the sidebar. A **Favorites** folder for the projects you check every day is a common start. Each project sits in one folder at a time, and every project also appears under the built-in **All Projects** group. Projects that are in no folder are listed under **Not Categorized**, shown between your own folders and **All Projects**.

Folders, their names, icons and colors, and the folder each project is in are stored with your synced data. They follow you to every device that shares the same database.

## Hiding the Sidebar

The sidebar only appears on the [Project List Page](<ProjectListPage.md>), and you can put it away when you want the full window for the project cards. Choose **Show Project Folders** from the app menu's **View** group, or click the panel button near the bottom of the icon rail. Either one toggles the panel, and Project Notes remembers the choice for the next launch. The rail button is dimmed on other sections, since there is no sidebar to hide there.

## Showing One Folder's Projects

Click a folder's name in the sidebar to filter the project list to that folder. The header stays highlighted while the filter is on. Click the same name again, or click **All Projects**, to show every project. **Not Categorized** works the same way and shows the projects that are in no folder.

The folder filter is an ordinary column filter on the project list, so the **Filter** button lights up and the filter shows under **Folder** in the filter editor. Other filters and the quick search still apply on top of it. While the list shows one folder, the sidebar keeps listing every folder's projects, so you can switch folders with one click.

If a project is open, clicking a folder name takes you back to the project list.

## Collapsing a Folder

Click the arrow at the right end of a folder header to collapse or expand that group. Clicking anywhere else on the header filters the list instead. With the keyboard, move to a header with the arrow keys, press **Right** to expand it, **Left** to collapse it, and **Enter** to filter by it.

Collapsed groups are remembered on each computer separately. Collapsing a folder on one device leaves it open on your others.

## Creating a Folder

Create folders from **Settings > Project Folders**, or right-click any folder header in the sidebar and choose **New Folder…**.

**To create a folder in Settings:**

1. Open **Settings** and go to the **Project Folders** section.
2. Type a name into the **New folder name…** field.
3. Click **Add Folder**.

The new folder appears in the Settings list and as a group in the project sidebar, starting with a star icon and the first color swatch. Folder names must be unique; a name that differs from an existing one only by capitalization is refused.

## Renaming a Folder and Changing Its Icon or Color

In **Settings > Project Folders**:

- Click a folder's name to edit it. Press **Enter** or click elsewhere to save, or **Escape** to put the old name back.
- Click the folder's icon to choose another one.
- Click a color swatch to change its color.

From the sidebar, right-click the folder header and choose **Edit Folder…**. One dialog changes the name, icon and color together.

## Putting Projects into Folders

Drag a project row in the sidebar and drop it onto a folder header. The project moves into that folder and leaves the one it was in. Dropping a project onto **Not Categorized** or **All Projects** takes it out of its folder. Each folder header shows a live count of the projects in it.

The **Move To Folder** dialog does the same thing without dragging:

1. Right-click the project. This works in the sidebar, on its card on the [Project List Page](<ProjectListPage.md>), or on its own [Project Page](<ProjectPage.md>). Choose **Move To…** and the **Move To Folder** dialog opens.
2. Click the destination folder. The dialog closes with no separate confirm step.

Choosing **Not Categorized** at the bottom of the list takes the project out of its folder.

A duplicated project lands in the same folder as the original.

## Deleting a Folder

Click the trash icon next to a folder in **Settings > Project Folders**. Only the folder is removed. Its projects move to **Not Categorized**.

## Upgrading from Earlier Versions

Before version 6.5.0, folders were saved as a single setting, and a project could be in several folders. The first time 6.5.0 opens your database, it moves your folders into their own table. A project that was in several folders keeps the first one it was listed in. The old setting is then removed from your synced settings. Collapsed groups start out expanded after the upgrade.

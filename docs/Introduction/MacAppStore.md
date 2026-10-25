# Mac App Store Version

Project Notes is available on the Mac App Store and as a direct download from the Project Notes website. Both versions have the same notes, projects, reports, and plugin support. The App Store version runs in a macOS sandbox, which is a set of limits Apple places on apps from the store. Those limits change a few features, described below.

## Choosing a version

| If you want to | Install |
| :--- | :--- |
| Get updates automatically through the App Store | Mac App Store |
| Start other programs from plugins, or use Thunderbird | Direct download |
| Keep your data in your home folder | Direct download |

Before you switch versions, read [Moving your data](#moving-your-data).

## Updates

The App Store installs updates for you. **Help > Check for Updates** does not appear in this version, because the App Store already handles it.

## Choosing folders

The sandbox lets Project Notes read and write only the folders you choose. You choose a folder once, and Project Notes remembers it the next time you open the app.

Choose a folder in any of these places:

- **Project Folders:** Open a project, go to **Files & Folders**, and click **Browse Folder**. Use this instead of typing a path.
- **File Finder:** Open **Settings > File Finder** and add the folders you want searched.
- **Plugin files:** When a plugin asks for a file or folder, pick it in the window that opens.

A path you type by hand, or one copied from a database made on another computer, does not work until you pick that folder once. If a folder stops opening, pick it again with **Browse Folder** or **Settings > File Finder**.

Both versions have **Browse Folder**, so you can use the same steps on either one.

## Plugins

Plugins work in the App Store version, with two differences.

- **Plugins that start programs do not run.** The App Store version cannot open other applications. Plugins that use the `process` feature stop with an error that says launching programs is not available in this build. The Script Editor menu is also missing.
- **Adding your own plugins.** Your plugins folder is inside the app's container, not in your home folder. To add a plugin, open **Settings > Plugins** and click **Install Plugin…**. Project Notes copies the plugin into the folder for you. Click **Open Plugins Folder** to see it.

Plugins you install stay off until you review what they can do and turn them on. This works the same way in both versions. For plugin development details, see [JavaScript Plugins](<../PluginsOverview/JavaScriptPlugins.md>).

## Email, meetings, and calendars

The App Store version can create email drafts and calendar events in Apple's Mail and Calendar apps. You review each draft and send it yourself. Nothing is sent without you.

The Thunderbird integration is not in the App Store version. To use Thunderbird, install the direct download.

See [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>) for the steps.

## Moving your data

The App Store version stores its data in its own sandbox folder:

```
~/Library/Containers/com.projectnotespro.projectnotesmobile/Data/
```

The first time you open the App Store version, it looks for data from the direct-download version. If it finds some, it asks whether to copy it over. Click **Copy My Data** to bring across your database, plugins, and settings. If the downloaded version is open, Project Notes asks you to quit it first so nothing changes during the copy.

Both versions install as **Project Notes** in your Applications folder, so you keep one or the other. Your data is copied, not moved: the downloaded version's data stays where it was, and you can go back to it by installing the download again. The two copies don't update each other, so keep working in one version.

After the copy, choose your project folders and File Finder locations again, because the App Store version can open only folders you pick. If cloud sync asks you to sign in, sign in again.

If you clicked **Start Empty**, or you're moving data the other way, use one of these methods:

- **Cloud sync:** Sync from the old version with [Cloud Sync](<../InterfaceOverview/RemoteHost.md>) (Project Notes Pro), then sync in the new version.
- **XML export and import:** In the old version, choose **Export XML…** from the application menu. In the new version, choose **Import XML…** from the same menu.

After you move your data, check that your project folders still open. Pick any folder again if it does not.

## Getting help

If a feature does not work in the App Store version, check the **Error Log** (see [Error Log](<../InterfaceOverview/ErrorLog.md>)). If the feature needs an action the sandbox blocks, install the direct download instead.

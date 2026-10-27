# Releasing on the Microsoft Store

The Store build is an MSIX package of the same executable and files the GitLab installer ships. It has no Remote Host. It runs as a full trust desktop app, so nothing in the code is sandboxed. The one runtime difference: when the process has a package identity, `main.cpp` turns the in-app updater off, because the Store updates the app.

## One-time setup

1. Register a developer account in Partner Center and reserve the name "Project Notes".
2. On the app's *Product identity* page, copy three values and keep them in the environment where you build:

   ```powershell
   $env:PN_MSIX_IDENTITY_NAME          = '<Package/Identity/Name>'
   $env:PN_MSIX_PUBLISHER              = 'CN=<Package/Identity/Publisher>'
   $env:PN_MSIX_PUBLISHER_DISPLAY_NAME = '<Package/Properties/PublisherDisplayName>'
   ```

3. Install the Windows 10/11 SDK (for `makeappx.exe` and `signtool.exe`) alongside the Visual Studio 2022 and Qt kit the GitLab build uses.

## Each release

1. Set the version in the top-level `CMakeLists.txt`. The package version is `x.y.z.0`; the Store reserves the last number.
2. Build the package:

   ```powershell
   packaging\windows\store\build_store.ps1
   ```

   It runs the `deploy` target, copies the result, draws the tile logos from `docs/img/logo1024.png`, writes `AppxManifest.xml`, and packs `packaging\windows\store\output\ProjectNotes-<version>-x64.msix`.
3. Upload the `.msix` to a new submission in Partner Center. The Store signs the package, so it is not signed here.

## Testing before upload

```powershell
packaging\windows\store\build_store.ps1 -Local
```

This signs with a throwaway certificate and prints the two commands that trust it and install the package. Remove an installed copy of the GitLab build first: both use the `Project Notes.exe` name and the same data folder logic, and the packaged app keeps its own copy of writes under `%LOCALAPPDATA%\Packages\`.

## Things to know for review

- **runFullTrust.** Partner Center asks why. Project Notes is a Qt desktop app that reads and writes the user's chosen folders and talks to Outlook and Office through COM.
- **Data is separate.** A packaged app's writes to AppData go to a per-package location, so a user moving from the GitLab build does not see their database. Use the app's export and import, or copy `database\ProjectNotes.db` by hand.
- **Store screenshots and description** are entered in Partner Center. They are not part of the package.

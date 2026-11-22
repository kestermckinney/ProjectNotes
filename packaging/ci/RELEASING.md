# Automated releases (GitHub Actions)

Pushing a version tag such as `v6.5.3` starts two workflows:

- `.github/workflows/release.yml` builds Windows, macOS and iOS, attaches the direct downloads to the GitHub release, and submits the store builds for review.
- `.github/workflows/flatpak-release.yml` builds and publishes Linux. See `packaging/linux/RELEASING.md`.

Run **Release** by hand from the Actions tab to build everything without touching a release or a store. Its artifacts stay for 14 days.

## What each job does

| Job | Runner | Output |
| --- | --- | --- |
| `windows` | windows-2022 | `ProjectNotes-<ver>-Windows-x64-Setup.exe`, `ProjectNotesRemoteHost-<ver>-Windows-x64-Setup.exe`, the `.msix` |
| `macos` | macos-15 | Notarized `.dmg` and the updater's `-macOS.zip` (`build_installer.sh --all`) |
| `macos-appstore` | macos-15 | Signed Mac App Store `.pkg` (`build_appstore.sh`) |
| `ios` | macos-15 | Signed `.ipa` (`packaging/ios/build_appstore.sh`) |
| `github-release` | ubuntu | Uploads the Windows installers, `.dmg` and `.zip` to the release, creating a draft from `packaging/RELEASE_NOTES.md` if none exists |
| `app-store-ios`, `app-store-macos` | macos-15 | Uploads the build, waits for App Store Connect to process it, then submits the version for App Review with the text in `STORE_WHATS_NEW.txt` |
| `microsoft-store` | windows | Uploads the `.msix` to a new Partner Center submission and commits it for certification |

Approved App Store versions are not released automatically. Release them in App Store Connect. The GitHub release stays a draft until you publish it, and the in-app updater ignores it until then.

The macOS Remote Host installer (`build_remote_host_installer.sh`) is not built by the workflow.

## Before tagging

1. Set the version in the top-level `CMakeLists.txt` and in `PRODUCT_VERSION` in both `packaging/windows/*.nsi` files. The `prepare` job fails if they differ from the tag.
2. Rewrite `packaging/ci/STORE_WHATS_NEW.txt`. Its first line must name the new version, and it must stay under 4000 characters. Both App Store listings show it as "What's New".
3. Update `packaging/RELEASE_NOTES.md` for the GitHub release.
4. Push SqliteSyncPro first. Every job builds the tip of its default branch at the time the workflow starts.

## One-time setup

### Repository secrets

Settings > Secrets and variables > Actions > Secrets.

| Secret | Contents |
| --- | --- |
| `APPLE_DEVELOPER_ID_P12` | Base64 of one `.p12` holding **Developer ID Application** and **Developer ID Installer**, each with its private key |
| `APPLE_DEVELOPER_ID_P12_PASSWORD` | Its export password |
| `APPLE_DISTRIBUTION_P12` | Base64 of one `.p12` holding **Apple Distribution** and **Mac Installer Distribution** (or **3rd Party Mac Developer Installer**), each with its private key. The iOS and Mac App Store jobs share it |
| `APPLE_DISTRIBUTION_P12_PASSWORD` | Its export password |
| `MACOS_APPSTORE_PROFILE` | Base64 of the Mac App Store provisioning profile for `com.projectnotespro.projectnotesmobile` |
| `IOS_APPSTORE_PROFILE` | Base64 of an iOS **App Store Connect** distribution profile for `com.projectnotespro.projectnotesmobile`, made in the developer portal (Profiles > +) for the Apple Distribution certificate in `APPLE_DISTRIBUTION_P12`. The iOS export signs manually with it, because automatic signing on CI needs an Admin API key |
| `ASC_API_KEY_ID` | App Store Connect API key ID |
| `ASC_API_ISSUER_ID` | Its issuer ID |
| `ASC_API_KEY_P8` | The text of `AuthKey_<KEYID>.p8` |
| `MSSTORE_TENANT_ID` | Microsoft Entra tenant ID linked to Partner Center |
| `MSSTORE_SELLER_ID` | Partner Center seller ID |
| `MSSTORE_CLIENT_ID` | Client (application) ID of the Entra app |
| `MSSTORE_CLIENT_SECRET` | A client secret of that app |

Set one repository **variable** too: `MSSTORE_PRODUCT_ID`, the Store ID of Project Notes Pro (Partner Center > the app > Product identity, something like `9NXXXXXXXXXX`).

To export a `.p12`, select the certificates in Keychain Access (expand each to check it has its private key), choose File > Export Items, and set a password. Then copy it as base64:

```bash
base64 -i Certificates.p12 | pbcopy
base64 -i ~/Library/Developer/Xcode/UserData/Provisioning\ Profiles/<profile>.provisionprofile | pbcopy
```

`build_appstore.sh` prints the profile it picks when run locally, which is the one to upload.

### App Store Connect API key

Users and Access > Integrations > App Store Connect API. Create a team key with the **App Manager** role (or **Admin**). The workflow uses it to notarize, to let `xcodebuild` fetch the iOS App Store profile, to upload, and to submit for review.

### Microsoft Store

1. In Partner Center, Account settings > User management > Microsoft Entra applications, add an Entra app with the **Manager** role. Create a client secret for it.
2. The first submission of the app must be made by hand in Partner Center. `msstore publish` only updates an existing listing.

### Environments

Create a `store-release` environment (Settings > Environments). The three store jobs run in it. With required reviewers added, each submission waits for approval in the run's page. Without reviewers they run as soon as their build finishes.

## When something fails

- **Retry a job.** Use "Re-run failed jobs". A store job that already uploaded will fail on upload, since App Store Connect refuses a build number twice; rerun the whole workflow instead to get a new build number.
- **Build locally instead.** The workflow calls the same scripts described in `packaging/macos/AppStoreReleasing.md`, `packaging/windows/store/Releasing.md` and `packaging/ios/build_appstore.sh`.
- **Qt version.** `QT_VERSION` at the top of `release.yml` picks the Qt kit for macOS and iOS, `QT_VERSION_WINDOWS` for Windows. Windows stays on 6.10 until aqtinstall can install Qt 6.11 for Windows, whose repository is split per compiler.

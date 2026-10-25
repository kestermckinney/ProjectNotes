# Releasing on the Mac App Store

Project Notes for macOS ships two ways. The direct download (`build_installer.sh`, published on GitLab) is a notarized, Developer ID signed installer with its own updater. The Mac App Store build is sandboxed, has no updater, and is built from the same source with `-DPN_APP_STORE=ON`. Both use `com.projectnotespro.projectnotesmobile`, the iOS app's bundle ID (`PN_MACOS_BUNDLE_ID` in the top-level `CMakeLists.txt`), because the Mac app ships in the iOS app's App Store Connect record ("Project Notes Pro"). One listing and one purchase cover iPhone, iPad and Mac. Releases before 6.5.0 used `com.projectnotespro.ProjectNotes` for the app and its installer package. The installer package now uses the new ID too, so macOS records it as a new package rather than an upgrade, and the direct download's `preinstall` script removes the old app before installing.

The store build is sandboxed, so its data lives in a container and doesn't share the direct download's. On its first start it offers to copy that data (`ProjectNotesDesktop/DirectDownloadMigration.mm`).

Both builds run on macOS 13 or later, on Apple silicon and Intel. `build_common.sh` holds the build steps they share.

Two scripts handle the store build:

- `build_appstore.sh` builds static Hunspell and OpenSSL, configures and builds the App Store flavor, checks the bundle, signs it, wraps it in a `.pkg`, and uploads it when given an API key.
- `upload_appstore.sh` validates and uploads a `.pkg` that already exists. Use it to retry an upload without rebuilding.

`AppStoreReviewNotes.md` has the text for App Review and the reason for each entitlement.

## One-time setup

**Tools.** Xcode, the Qt macOS kit (`~/Qt/6.11.1/macos` by default, or set `QT_MACOS`), and a vcpkg checkout (`~/Documents/Projects/vcpkg` by default, or set `VCPKG_ROOT`). vcpkg builds Hunspell with autotools, so `brew install autoconf automake libtool pkg-config` if it complains.

**Certificates.** The Developer ID certificates used for the direct download are rejected by the App Store. Two more are needed in the login keychain, both for team 2PY624KHXH:

- *Apple Distribution* signs the app.
- *Mac Installer Distribution* signs the package. In the keychain it appears as "3rd Party Mac Developer Installer".

Create both in Xcode > Settings > Accounts > Manage Certificates (the + button). The iOS build doesn't need them on disk because Xcode signs it in the cloud. The Mac build signs with `codesign` directly, so the private keys must be local.

**App ID.** The iOS app's App ID, `com.projectnotespro.projectnotesmobile`, already exists in the developer portal under Identifiers. Open it and check that macOS is among its platforms. Xcode-created IDs usually cover all platforms. No extra capabilities are needed, because the sandbox entitlements don't need portal switches.

**Provisioning profile.** Under Profiles, create a *Mac App Store Connect* distribution profile for that App ID and the Apple Distribution certificate. Then use Xcode > Settings > Accounts > Download Manual Profiles. The script picks the newest unexpired matching profile from Xcode's profile folder on its own. Set `PN_PROVISIONING_PROFILE` to use a specific file instead.

**App Store Connect record.** Use the existing "Project Notes Pro" record. It already has a macOS platform, and Mac builds appear under its TestFlight tab. Don't create a new record.

**API key.** The iOS upload uses the same key. It lives in App Store Connect > Users and Access > Integrations, with at least the App Manager role, and its file is saved as `~/.appstoreconnect/private_keys/AuthKey_<KEYID>.p8`.

## Each release

1. Set the version in the top-level `CMakeLists.txt` (`project(ProjectNotes VERSION x.y.z)`). It becomes the App Store version string.
2. Build and upload:

   ```bash
   ASC_API_KEY_ID=XXXXXXXXXX ASC_API_ISSUER=xxxxxxxx-xxxx-... \
       packaging/macos/build_appstore.sh
   ```

   Leave out the two variables to stop after the `.pkg` is built, then upload it with Transporter or `upload_appstore.sh`. Add `--validate-only` to run App Store Connect's checks without uploading.
3. When processing finishes, the build shows up under TestFlight. Test it there, then attach it to a new version on the App Store tab and submit it for review.

The build number defaults to a UTC timestamp. Pass one (`build_appstore.sh 42`) to use your own sequence. App Store Connect refuses any build number it has seen before for the app.

To try the package locally before uploading, install it the way the store does:

```bash
sudo installer -store -pkg packaging/macos/output-appstore/ProjectNotes-<version>-<build>-AppStore.pkg -target /
```

## What the script checks

Before signing, both release scripts stop if any binary in the bundle:

- lacks an arm64 or x86_64 slice,
- needs a newer macOS than the app's minimum (13.0), or
- loads a library from `/opt/homebrew` or `/usr/local` instead of from the bundle.

Homebrew libraries trip the last two checks most often. They target the macOS they were built on and live outside the bundle. Homebrew's builds are also arm64 only. So Hunspell and OpenSSL come from vcpkg instead, built once per architecture with the `pn-arm64-osx` and `pn-x64-osx` triplets (`vcpkg-triplets/`) and merged with `lipo`. The unused SQL drivers are removed too, since the ODBC one pulls in Homebrew's `libiodbc`.

The minimum macOS appears in four places that must agree: `MIN_MACOS` in `build_common.sh`, both triplets, `distribution.xml` (direct download only), and `CMAKE_OSX_DEPLOYMENT_TARGET` in `CMakeLists.txt`.

## Common rejections

| Message | Cause |
| --- | --- |
| Missing application identifier, or the identifier does not match the profile | The profile is for another App ID or team. The script adds `com.apple.application-identifier` to the entitlements from `TEAM_ID` and the bundle ID. |
| Invalid signature, or not signed with a distribution certificate | Developer ID or Apple Development identity. Check `APP_IDENTITY`. |
| Package not signed with an installer certificate | A Developer ID Installer identity was used. `upload_appstore.sh` checks for this before uploading. |
| Redundant binary upload | The build number was used before. |
| `LSMinimumSystemVersion` missing | The bundle came from an old configure. Rebuild without `--skip-build`. |

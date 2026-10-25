# AGENTS.md

Compact, repo-specific guidance an automated agent will likely miss. Keep it short — each line answers "Would an agent likely miss this without help?" If not, it's omitted.

Summary
- Qt/C++ desktop app with a JavaScript (QJSEngine) plugin system. CMake-based build; headless Qt tests under tests/desktop.

Quick build (common, exact):
- mkdir build && cd build
- cmake .. -DCMAKE_BUILD_TYPE=Debug|Release [see OS notes]
- cmake --build . --config Release   # on multi-config generators specify --config
- Default target is `ProjectNotes` = the QML frontend (source dir ProjectNotesDesktop/, output <build>/ProjectNotesDesktop/). The legacy Widgets app and its `BUILD_WIDGETS_LEGACY` option have been removed — `ProjectNotes` is the only desktop frontend. The app still links Qt::Widgets and runs a QApplication because ProjectNotesCore reports database errors through QMessageBox; don't replace it with QGuiApplication.

High-signal gotchas
- SqliteSyncPro is a required sibling: CMake calls add_subdirectory(../SqliteSyncPro/src). Put that repo at ../SqliteSyncPro or CMake will fail.
- The data layer (databaseobjects.cpp, sqlquerymodel.cpp, etc., all still at repo root) is compiled into a static lib by ProjectNotesCore/CMakeLists.txt and linked by both the desktop QML app and ProjectNotesMobile. A change to those files affects the iOS app too, even though nothing in this build tree builds it.
- ProjectNotesMobile/ (iOS/iPadOS companion app) has its own top-level CMakeLists.txt and is NOT part of this build — don't add_subdirectory it or expect `cmake --build .` to touch it. It's configured separately with an iOS toolchain/Xcode generator and a hardcoded vcpkg OpenSSL path for the maintainer's Mac, then archived via packaging/ios/build_appstore.sh. Its C++ (AppController, MobileSettings, TextFormatter) and QML live under ProjectNotesMobile/, separate from ProjectNotesDesktop/.
- Bundled plugins are folders under ./plugins (plugin.json + main.mjs). On macOS Debug and on Windows/Linux the build symlinks ./plugins into the binary dir, so edits are visible immediately (plugins hot-reload). On macOS Release run the custom target sync_plugins to copy real files into the .app bundle (cmake --build . --target sync_plugins). office-tools is Windows-only and is left out of macOS and Linux packages.
- extras/ holds plugins that ship separately as .pnplugin packages (packaging/plugins/build-pnplugin.sh); nothing in the build installs them.
- There is no embedded Python any more. Old releases' Python plugin files are removed on upgrade by packaging/RemoveRetiredPlugins.cmake.in, packaging/windows/remove_retired_plugins.nsh and packaging/macos/projectnotes-scripts/preinstall; keep the three lists in step.
- QJSEngine is roughly ES2016: no async/await, object spread or catch without a binding. Plugins use generator functions (`function*` / `yield promise`) that the prelude drives.
- Report PDFs are printed with QTextDocument + QPdfWriter (ProjectNotesEmailRendering/TextDocumentReportRenderer).
- The Windows deploy target expects SQLITESYNCPRO_BUILD_DIR; the default is filled in CMakeLists but you will often need to override it when packaging.
- Hunspell is optional at configure time but CMake looks for hunspell headers/libs; missing hunspell will affect spellcheck features. CMake prints Hunspell include/lib locations during configure.
- On macOS release the build runs macdeployqt and re-signs the bundle — read CMakeLists lines around macOS post-build steps before attempting to create distributables.
- The CMake build generates version.h from version.h.in — small but useful to know when changing version numbers.

Runtime/UX notes useful for tests and debugging
- Single-instance enforced via RunGuard; starting a second instance shows a modal critical message and exits.
- Developer profile: run the app with --developer-profile NAME to use a separate settings profile (main.cpp parses this option).
- PN_PLUGIN_USER_DIR and PN_PLUGIN_BUNDLED_DIR override the plugin folders (the UI tests point both at empty directories).

Tests
- Configure a separate build with -DBUILD_QML_DESKTOP_TESTS=ON -DBUILD_QML_DESKTOP=OFF, build, then run ctest with QT_QPA_PLATFORM=offscreen. tst_pluginhost runs every bundled plugin and extras/ifs-cloud against fake services and servers.

Where to read first (high value):
- CMakeLists.txt (build rules, SqliteSyncPro dependency, sync_plugins + deploy targets)
- ProjectNotesPlugins/ (plugin host) and docs/PluginsOverview/JavaScriptPlugins.md (plugin API)
- main.cpp (--developer-profile flag, single instance guard)
- README.md and docs/ for developer/system requirements

What is NOT here
- No lint or formatting rules in the repo.

If you need more context
- If packaging for macOS or Windows, read the platform-specific sections in CMakeLists carefully — they contain deploy/packaging commands you must run rather than guess.

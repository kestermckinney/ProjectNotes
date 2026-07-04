# AGENTS.md

Compact, repo-specific guidance an automated agent will likely miss. Keep it short — each line answers "Would an agent likely miss this without help?" If not, it's omitted.

Summary
- Qt/C++ desktop app with an embedded Python plugin system. CMake-based build; no test or linter suites.

Quick build (common, exact):
- mkdir build && cd build
- cmake .. -DCMAKE_BUILD_TYPE=Debug|Release [see OS notes]
- cmake --build . --config Release   # on multi-config generators specify --config

High-signal gotchas
- SqliteSyncPro is a required sibling: CMake calls add_subdirectory(../SqliteSyncPro/src). Put that repo at ../SqliteSyncPro or CMake will fail.
- Python dev headers are required and CMake expects specific versions per OS: Windows/macOS -> Python 3.13 (the CMakeLists sets Python3 3.13); Linux defaults to 3.14. Use -DPython3_ROOT_DIR on macOS if you installed python.org framework (CMakeLists mentions this).
- The build links against the Python release library (CMake sets Python3_LIBRARIES from the release variant). Don't assume the debug python libs are used.
- The app expects PyQt6 wheel files under ./site-packages when run from the build/bundle (main.cpp adds ./site-packages/PyQt6 paths). When running from the build output ensure site-packages/PyQt6 exists (or the bundle contains a Python.framework in macOS release builds).
- Plugins are under ./plugins and threads under ./threads. On macOS Debug the build symlinks these into the app binary dir (fast edit → visible). On macOS Release you must run the custom target sync_plugins to copy real files into the .app bundle (cmake --build . --target sync_plugins). On non-mac platforms the build creates symlinks to the source plugins in the binary dir so editing source plugins will usually be visible immediately.
- There is a custom deploy target for Windows that expects several CMake cache variables (PYTHON_EMBED_DIR, PYTHON_PACKAGES_DIR, SQLITESYNCPRO_BUILD_DIR). The defaults are filled in CMakeLists but you will often need to override them when packaging.
- Hunspell is optional at configure time but CMake looks for hunspell headers/libs; missing hunspell will affect spellcheck features. CMake prints Hunspell include/lib locations during configure.
- On macOS release the build runs macdeployqt and bundles Python.framework via packaging/macos/BundlePythonFramework.cmake — packaging is non-trivial; read CMakeLists lines around macOS post-build steps before attempting to create distributables.
- The CMake build generates version.h from version.h.in — small but useful to know when changing version numbers.

Runtime/UX notes useful for tests and debugging
- Single-instance enforced via RunGuard; starting a second instance shows a modal critical message and exits.
- Developer profile: run the app with --developer-profile NAME to use a separate settings profile (main.cpp parses this option).
- Main process may set QTWEBENGINEPROCESS_PATH when QtWebEngine helper executable is found under site-packages — keep that path intact if you move the bundle.

Where to read first (high value):
- CMakeLists.txt (build rules, Python version expectations, SqliteSyncPro dependency, sync_plugins + deploy targets)
- main.cpp (site-packages paths, --developer-profile flag, single instance guard)
- README.md and docs/ for developer/system requirements

What is NOT here
- No unit-test or CI test invocation (there's no automated test framework). Don't look for pytest/ctest glue unless you add it.
- No lint or formatting rules in the repo.

If you need more context
- If packaging for macOS or Windows, read the platform-specific sections in CMakeLists carefully — they contain deploy/packaging commands you must run rather than guess.

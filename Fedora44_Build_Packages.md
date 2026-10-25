# Required Packages for Fedora 44

These packages build the desktop app (`ProjectNotes`) against Fedora's system Qt 6. Python and Qt WebEngine are no longer needed. Plugins run in QJSEngine, which is part of Qt Declarative.

## Build Tools
```bash
sudo dnf group install c-development
sudo dnf install cmake gcc-c++ make
```

## Qt 6
```bash
sudo dnf install qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtdeclarative-devel qt6-qtcharts-devel
```

- `qt6-qtbase-devel` provides Core, Gui, Widgets, Sql, Xml, Network and DBus.
- `qt6-qtbase-private-devel` provides `Qt6::CorePrivate`, which the plugin host and desktop app link.
- `qt6-qtdeclarative-devel` provides Qml, Quick and Quick Controls 2. Fedora no longer ships a separate `qt6-qtquickcontrols2-devel`.
- `qt6-qtcharts-devel` is required by `SqliteSyncProLib`, even though ProjectNotes itself draws no charts.

## Other Libraries
```bash
sudo dnf install hunspell-devel openssl-devel
```

Hunspell handles spell checking. The en_US and en_GB dictionaries come from the repository's `dictionary/` folder, so no `hunspell-en` package is needed. OpenSSL is required by `SqliteSyncProLib`.

## SqliteSyncPro
CMake expects a checkout of SqliteSyncPro next to this repository, at `../SqliteSyncPro/src`.

## Using the Qt Online Installer Instead
The Sublime Text and Qt Creator configurations build against the Qt in `~/Qt/6.11.1/gcc_64`, not the system Qt. That install already includes Charts, so the `qt6-*` packages above are optional there. Pass `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.11.1/gcc_64` when configuring, and set `LD_LIBRARY_PATH=$HOME/Qt/6.11.1/gcc_64/lib` when running. The app is linked with an `$ORIGIN`-only RPATH, so without that it loads the system Qt.

# Required Packages for Fedora 44

To successfully build this project on Fedora 44, install the following packages:

## Python Development
```bash
sudo dnf install python3-devel python3-pyqt6 python3-pyqt6-base python3-pyqt6-devel python3-pyqt6-webengine
```

## Qt6 Development (Required for this project)
```bash
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtquickcontrols2-devel qt6-qtwebengine-devel qt6-qtcharts-devel
```

## Hunspell (for spellcheck features)
```bash
sudo dnf install hunspell hunspell-devel
```

## Additional Development Tools
```bash
sudo dnf groupinstall "C Development Tools and Libraries"
sudo dnf install cmake gcc gcc-c++ make
```

## Optional but Recommended
```bash
# For better Python plugin support, you might also want:
sudo dnf install python3-pip
```

## Note on Python Version
This project uses Python 3.12 or later (as specified in CMakeLists.txt). Fedora 44 should have Python 3.12 available by default, but if you encounter issues with version mismatches, you may need to install Python 3.13:

```bash
sudo dnf install python3.13 python3.13-devel
```

The key packages needed for your build to work properly are:
- python3-devel (for Python development headers)
- python3-pyqt6 and related packages (PyQt6 bindings)  
- qt6-qtbase-devel and other Qt6 components
- hunspell and hunspell-devel (for spell checking)
- C development tools and cmake
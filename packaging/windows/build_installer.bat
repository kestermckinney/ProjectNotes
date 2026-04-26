@echo off
setlocal enabledelayedexpansion

rem Initialize VS 2022 environment so windeployqt can find MSVC redistributables
if not defined VCINSTALLDIR (
    set "_VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    if not exist "!_VCVARS!" set "_VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    if not exist "!_VCVARS!" set "_VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    if exist "!_VCVARS!" (
        call "!_VCVARS!"
    ) else (
        echo WARNING: Could not find vcvars64.bat - MSVC redistributables may not be deployed.
    )
)

set BUILD_CONFIG=Desktop_Qt_6_10_0_MSVC2022_64bit-Release
set BUILD_DIR=%~dp0..\..\build\%BUILD_CONFIG%

echo.
echo === Building Project Notes installer ===
echo Build dir : %BUILD_DIR%
echo.

rem Stage all deploy files (windeployqt, Python env, site-packages, plugins...)
cmake --build "%BUILD_DIR%" --target deploy
if errorlevel 1 (
    echo.
    echo ERROR: cmake deploy target failed.
    exit /b 1
)

rem Compile the NSIS installer (deploy target already ran above, !system in NSI is a no-op)
makensis "%~dp0setupscript.nsi"
if errorlevel 1 (
    echo.
    echo ERROR: makensis failed.
    exit /b 1
)

echo.
echo Done. Installer: %~dp0ProjectNotes-Setup64.exe
endlocal

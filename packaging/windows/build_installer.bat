@echo off
setlocal

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

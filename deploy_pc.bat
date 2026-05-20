@echo off

REM ============================================================
REM   IDSP Release Packaging Script
REM   Qt5 + MinGW + QML project (Windows)
REM ============================================================

REM --- Set your paths here ---
set SRC=D:\workspace\project\StabilityAnalyzer\StabilityAnalyzer_PC\bin-mingw
set DIST=%~dp0StabilityAnalyzer_PC_Release
set QT_BIN=D:\Qt\Qt5.12.12\5.12.12\mingw73_32\bin
set MINGW_BIN=D:\Qt\Qt5.12.12\Tools\mingw730_32\bin
set QT_QML=D:\Qt\Qt5.12.12\5.12.12\mingw73_32\qml
set QT_PLUGINS=D:\Qt\Qt5.12.12\5.12.12\mingw73_32\plugins

echo ============================================
echo   IDSP Release Packaging Script
echo ============================================
echo.

REM --- Pre-check ---
if not exist "%SRC%\ANALYZER.exe" (
    echo [ERROR] Cannot find %SRC%\ANALYZER.exe
    echo Please check SRC path and build Release first!
    pause
    exit /b 1
)

REM --- Clean old output ---
if exist "%DIST%" (
    echo Deleting old release directory...
    rmdir /s /q "%DIST%"
)
mkdir "%DIST%"


REM --- [1] Copy main exe ---
echo [1/9] Copying main exe...
copy "%SRC%\ANALYZER.exe" "%DIST%\" >nul


REM --- [2] Copy Release DLLs (exclude Debug) ---
echo [2/9] Copying Release DLLs (excluding Debug)...
REM Must use /R for regex mode, otherwise $ is treated as literal char
for %%f in ("%SRC%\*.dll") do (
    echo %%f | findstr /R /I "d\.dll$" >nul
    if errorlevel 1 (
        copy "%%f" "%DIST%\" >nul
    )
)


REM --- [3] Copy QML plugin directory ---
echo [3/9] Copying QML plugins (customPlugin)...
if exist "%SRC%\customPlugin" (
    xcopy "%SRC%\customPlugin" "%DIST%\customPlugin\" /E /I /Y >nul
    echo   - Copied customPlugin directory
) else (
    echo   - customPlugin not found, skipping
)


REM --- [4] Copy config directories ---
echo [4/9] Copying config directories...
if exist "%SRC%\config" (
    xcopy "%SRC%\config" "%DIST%\config\" /E /I /Y >nul
    echo   - Copied config directory
) else (
    echo   - config not found, skipping
)

if exist "%SRC%\template_cn" (
    xcopy "%SRC%\template_cn" "%DIST%\template_cn\" /E /I /Y >nul
    echo   - Copied template_cn directory
)


REM --- [5] Run windeployqt ---
echo [5/9] Running windeployqt...
"%QT_BIN%\windeployqt.exe" --release --qmldir "%QT_QML%" "%DIST%\ANALYZER.exe"


REM --- [6] Copy MinGW runtime DLLs ---
echo [6/9] Copying MinGW runtime DLLs...
copy "%MINGW_BIN%\libgcc_s_dw2-1.dll" "%DIST%\" >nul
copy "%MINGW_BIN%\libstdc++-6.dll"    "%DIST%\" >nul
copy "%MINGW_BIN%\libwinpthread-1.dll" "%DIST%\" >nul


REM --- [7] Copy additional Qt DLLs ---
echo [7/9] Copying additional Qt DLLs...
copy "%QT_BIN%\Qt5SerialPort.dll" "%DIST%\" >nul 2>nul
copy "%QT_BIN%\Qt5Multimedia.dll" "%DIST%\" >nul 2>nul
copy "%QT_BIN%\Qt5VirtualKeyboard.dll" "%DIST%\" >nul 2>nul
copy "%QT_BIN%\Qt5Sql.dll" "%DIST%\" >nul 2>nul
copy "%QT_BIN%\Qt5Xml.dll" "%DIST%\" >nul 2>nul

if exist "%QT_QML%\QtQuick\VirtualKeyboard" (
    xcopy "%QT_QML%\QtQuick\VirtualKeyboard" "%DIST%\QtQuick\VirtualKeyboard\" /E /I /Y >nul
    echo   - Copied VirtualKeyboard QML module
)


REM --- [8] CRITICAL: Deploy QSQLITE driver plugin ---
echo [8/9] Deploying QSQLITE driver plugin...
REM
REM *** This is the ROOT CAUSE of missing database tables ***
REM
REM The database init has TWO paths:
REM   Path A: sqlite_orm sync_schema() -> creates 5 base tables (no QSQLITE needed)
REM   Path B: QSqlDatabase + QSQLITE  -> creates 3 extra tables + ALTER TABLE
REM
REM Without qsqlite.dll, Path B silently fails and you lose:
REM   - separation_layer_data table
REM   - instability_curve_data table
REM   - instability_segment_curve_data table
REM
if not exist "%DIST%\sqldrivers" mkdir "%DIST%\sqldrivers"
copy "%QT_PLUGINS%\sqldrivers\qsqlite.dll" "%DIST%\sqldrivers\" >nul 2>nul
if exist "%DIST%\sqldrivers\qsqlite.dll" (
    echo   - QSQLITE driver deployed OK
) else (
    echo   - [WARNING] QSQLITE driver deploy FAILED! Database will be incomplete!
    echo     Check path: %QT_PLUGINS%\sqldrivers\qsqlite.dll
)


REM --- [9] Ensure platform plugin ---
echo [9/9] Checking platform plugin...
if not exist "%DIST%\platforms\qwindows.dll" (
    if not exist "%DIST%\platforms" mkdir "%DIST%\platforms"
    copy "%QT_PLUGINS%\platforms\qwindows.dll" "%DIST%\platforms\" >nul 2>nul
    echo   - Supplemented platform plugin
)


REM --- Verify critical files ---
echo.
echo ============================================
echo Verifying critical files...
echo ============================================

set VERIFY_OK=1

if not exist "%DIST%\ANALYZER.exe" (
    echo   [X] ANALYZER.exe MISSING
    set VERIFY_OK=0
) else (
    echo   [OK] ANALYZER.exe
)

if not exist "%DIST%\SqlOrm.dll" (
    echo   [X] SqlOrm.dll MISSING
    set VERIFY_OK=0
) else (
    echo   [OK] SqlOrm.dll
)

if not exist "%DIST%\Qt5Sql.dll" (
    echo   [X] Qt5Sql.dll MISSING
    set VERIFY_OK=0
) else (
    echo   [OK] Qt5Sql.dll
)

if not exist "%DIST%\sqldrivers\qsqlite.dll" (
    echo   [X] sqldrivers\qsqlite.dll MISSING - Database will be incomplete!
    set VERIFY_OK=0
) else (
    echo   [OK] sqldrivers\qsqlite.dll
)

if not exist "%DIST%\platforms\qwindows.dll" (
    echo   [X] platforms\qwindows.dll MISSING - App will not start!
    set VERIFY_OK=0
) else (
    echo   [OK] platforms\qwindows.dll
)

if not exist "%DIST%\MainWindow.dll" (
    echo   [X] MainWindow.dll MISSING
    set VERIFY_OK=0
) else (
    echo   [OK] MainWindow.dll
)


REM --- Done ---
echo.
echo ============================================
if "%VERIFY_OK%"=="1" (
    echo Packaging complete! All critical files verified.
) else (
    echo Packaging complete but some files MISSING - check [X] above!
)
echo Release directory: %DIST%
echo.
echo IMPORTANT:
echo   1. Make sure you built Release config in Qt Creator
echo   2. If database is still wrong, check app log for "Driver not loaded"
echo ============================================
pause

@echo off
rem Start the client on the shard named in shard.conf.
rem
rem   play-ignis.cmd              - just play (double-click works too)
rem   play-ignis.cmd -fastlogin   - anything extra is passed to the client
rem
rem Run setup.cmd first; it is what puts the UO data in place.

setlocal enabledelayedexpansion
cd /d "%~dp0"

rem The client resolves uo_debug.cfg and its own writable files relative to the
rem working directory, not to argv[0]. The cd above is load-bearing.

set "SHARD_NAME=Ignis UO"
set "SHARD_HOST=uo.jmaul.co.uk"
set "SHARD_PORT=2593"
set "ORION_VERSION="

if exist "shard.conf" (
    for /f "usebackq tokens=1,* delims==" %%A in ("shard.conf") do (
        set "key=%%A"
        set "key=!key: =!"
        if /i "!key!"=="SHARD_NAME"    set "SHARD_NAME=%%B"
        if /i "!key!"=="SHARD_HOST"    set "SHARD_HOST=%%B"
        if /i "!key!"=="SHARD_PORT"    set "SHARD_PORT=%%B"
        if /i "!key!"=="ORION_VERSION" set "ORION_VERSION=%%B"
    )
)

if not exist "uo_debug.cfg" (
    echo The UO data has not been set up yet.
    echo.
    echo Run this first:
    echo     "%~dp0setup.cmd"
    echo.
    pause
    exit /b 1
)

echo Connecting to %SHARD_NAME% ^(%SHARD_HOST%:%SHARD_PORT%^)

if defined ORION_VERSION (
    OrionUO.exe "-login %SHARD_HOST%,%SHARD_PORT%" "-orionversion %ORION_VERSION%" %*
) else (
    OrionUO.exe "-login %SHARD_HOST%,%SHARD_PORT%" %*
)

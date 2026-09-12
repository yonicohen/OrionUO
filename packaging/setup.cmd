@echo off
rem Double-clickable wrapper around setup.ps1, so nobody has to know about
rem PowerShell execution policies.
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1" %*
if errorlevel 1 pause

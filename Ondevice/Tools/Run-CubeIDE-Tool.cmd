@echo off
chcp 65001 >nul
setlocal
set "FIRMWARE_PWSH="
for /f "delims=" %%P in ('where pwsh.exe 2^>nul') do if not defined FIRMWARE_PWSH set "FIRMWARE_PWSH=%%P"
if defined FIRMWARE_PWSH goto run
if exist "%ProgramFiles%\PowerShell\7\pwsh.exe" set "FIRMWARE_PWSH=%ProgramFiles%\PowerShell\7\pwsh.exe"
if defined FIRMWARE_PWSH goto run
if exist "%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\native\powershell\pwsh.exe" set "FIRMWARE_PWSH=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\native\powershell\pwsh.exe"
if defined FIRMWARE_PWSH goto run
echo ERROR: PowerShell 7 not found. Install PowerShell 7 or add pwsh.exe to PATH.
exit /b 1
:run
"%FIRMWARE_PWSH%" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%~dp0Run-CubeIDE-Tool.ps1" %*
exit /b %ERRORLEVEL%

@echo off
setlocal
cd /d "%~dp0"

set "PY=py"
where py >nul 2>&1
if errorlevel 1 set "PY=python"

%PY% -c "import numpy, serial, soundcard" >nul 2>&1
if errorlevel 1 (
  echo Installing dependencies...
  %PY% -m pip install -r requirements.txt
  if errorlevel 1 goto :error
)

%PY% m5clock_spectrum.py %*
set "RC=%ERRORLEVEL%"
if /i "%~1"=="--autostart" exit /b %RC%
echo.
pause
exit /b %RC%

:error
echo.
echo Install failed. Check Python and network, then run this file again.
if /i "%~1"=="--autostart" exit /b 1
pause
exit /b 1

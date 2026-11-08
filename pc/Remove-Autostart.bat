@echo off
set "LINK=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\M5Clock Spectrum.lnk"
if exist "%LINK%" (
  del "%LINK%"
  echo Autostart removed.
) else (
  echo Autostart was not installed.
)
pause

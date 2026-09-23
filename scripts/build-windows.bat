@echo off
REM ==================================================================
REM  Video Gem - Windows build helper (the Windows version of "make merge")
REM  Double-click this file. It combines VideoGem\ (the engine) and
REM  programs\default\ (the visuals) into build\VideoGem\, then opens
REM  the combined sketch in the Arduino IDE.
REM
REM  IMPORTANT: build\ is wiped and rebuilt every time you run this.
REM  Make your edits in VideoGem\ or programs\default\ with VS Code,
REM  not in the build\ copy that the Arduino IDE shows you.
REM ==================================================================
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\VideoGem"

if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%"

copy /y "%ROOT%\VideoGem\*.*" "%OUT%\" >nul
copy /y "%ROOT%\programs\default\*.ino" "%OUT%\" >nul
if exist "%ROOT%\programs\default\*.h" copy /y "%ROOT%\programs\default\*.h" "%OUT%\" >nul

echo.
echo  Merged sketch is in build\VideoGem
echo  Opening it in the Arduino IDE...
echo.
start "" "%OUT%\VideoGem.ino"
timeout /t 4 >nul

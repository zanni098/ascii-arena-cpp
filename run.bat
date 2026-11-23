@echo off
REM ASCII ARENA - build (if needed) and play. Double-click this file.
setlocal
cd /d "%~dp0"

if exist "build\ascii_arena.exe" goto :run

call "%~dp0build.bat"
if errorlevel 1 goto :failed

:run
mode con: cols=100 lines=34 >nul 2>nul
build\ascii_arena.exe
echo.
echo [run] game closed.
pause
exit /b 0

:failed
echo.
echo [run] build failed - see the messages above.
pause
exit /b 1

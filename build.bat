@echo off
REM AASDK Build Script for Windows
REM This is a Windows batch file wrapper for the build.sh script
REM For full functionality, use WSL or the DevContainer environment

setlocal EnableDelayedExpansion

echo ================================================
echo   AASDK Build Script (Windows)
echo ================================================
echo.

REM Check if we're in WSL
if exist "/proc/version" (
    echo Running in WSL environment - using bash script
    bash build.sh %*
    goto :end
)

REM Check if we're in a DevContainer or have bash available
where bash >nul 2>&1
if %errorlevel% == 0 (
    echo Using bash to run build script
    bash build.sh %*
    goto :end
)

REM If no bash available, provide instructions
echo ERROR: This project requires a Unix-like environment to build.
echo.
echo Recommended options:
echo   1. Use VS Code DevContainers (recommended)
echo      - Install Docker Desktop and VS Code Dev Containers extension
echo      - Open this project in VS Code
echo      - Ctrl+Shift+P ^> "Dev Containers: Reopen in Container"
echo.
echo   2. Use WSL (Windows Subsystem for Linux)
echo      - Install WSL: wsl --install
echo      - Navigate to this directory in WSL and run: ./build.sh
echo.
echo   3. Use Git Bash or MSYS2
echo      - Install Git for Windows (includes Git Bash)
echo      - Run this script from Git Bash terminal
echo.
echo For complete setup instructions, see BUILD.md
echo.
pause

:end
endlocal

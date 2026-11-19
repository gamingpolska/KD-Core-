@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title KD Core - Build (developer only)

where cmake >nul 2>&1
if errorlevel 1 (
  echo CMake is not installed or is not in PATH.
  echo This script is for developers only, not for customers.
  pause
  exit /b 1
)
where cl >nul 2>&1
if errorlevel 1 (
  echo MSVC compiler not found.
  echo Open "x64 Native Tools Command Prompt for VS" and run this file there.
  pause
  exit /b 1
)

if exist build rmdir /s /q build
cmake -S . -B build -G "NMake Makefiles"
if errorlevel 1 goto :failed
cmake --build build --config Release
if errorlevel 1 goto :failed

if not exist dist mkdir dist
copy /y build\KDCore.exe dist\KDCore.exe >nul
if errorlevel 1 goto :failed
echo.
echo Build completed: dist\KDCore.exe
echo This is a developer build step. Customers use KD_Core_Setup.exe.
pause
exit /b 0

:failed
echo.
echo Build failed. No installer was created.
pause
exit /b 1

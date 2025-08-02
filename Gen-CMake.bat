@echo off
setlocal enabledelayedexpansion

echo Building Toaster Engine with CMake...

:: Check CMake
cmake --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake not found in PATH
    pause
    exit /b 1
)

:: Create build directory and navigate to it
if not exist build mkdir build
cd build

:: Configure CMake project
:: Build CMake project for Visual Studio 2022 on Windows x64
:: Set the possible configurations to Debug, Release, and Dist
echo Configuring project...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_CONFIGURATION_TYPES="Debug;Release;Dist"

:: Check for errors in configuration
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b %ERRORLEVEL%
)

:: Build
echo Building Debug configuration...
cmake --build . --config Debug --parallel

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b %ERRORLEVEL%
)

echo SUCCESS: Build completed
echo.
echo Generated Visual Studio solution is in the build directory
pause
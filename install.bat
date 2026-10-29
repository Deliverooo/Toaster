@echo off

net session >nul 2>&1
if %errorLevel% == 0 (
    goto :adminTask
) else (
    echo Requesting administrative privileges...
    powershell -Command "Start-Process -FilePath '%0' -Verb RunAs"
    exit /b
)

:adminTask
cd /d "%~dp0"

setx TOASTER_SDK "C:\Program Files\ToasterSDK\3.0" /M

echo %PATH%

cmake -B build -S . --install-prefix "C:/Program Files/ToasterSDK/3.0/"
cmake --build build --config Debug
cmake --install build --config Debug
cmake --build build --config Release
cmake --install build --config Release

pause
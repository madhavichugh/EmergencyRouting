@echo off
cd /d "%~dp0"
title Emergency Response and Ambulance Routing System - DSA Lab
echo =======================================================
echo   EMERGENCY RESPONSE AND AMBULANCE ROUTING SYSTEM
echo   Jaypee Institute of Information Technology - DSA Lab
echo =======================================================
echo.

if not exist app.exe (
    echo [1/3] Compiling C++ backend (g++ -std=c++17)...
    g++ -std=c++17 main.cpp dispatcher.cpp -o app.exe
    if errorlevel 1 (
        echo Compilation failed! Please ensure MinGW g++ is in your PATH.
        pause
        exit /b 1
    )
    echo Compilation successful!
) else (
    echo [1/3] Precompiled executable app.exe found!
)

echo.
echo [2/3] Launching web browser at http://localhost:8080 ...
start http://localhost:8080

echo.
echo [3/3] Starting C++ Dispatch Engine and HTTP Server...
echo Press Ctrl+C in this window when you wish to stop the server.
echo.
app.exe
pause

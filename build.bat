@echo off
echo ==========================================
echo       Ahnreal Engine Clean Build
echo ==========================================

REM 1. Clean build directory
if exist build (
    echo [1/3] Removing existing build directory...
    rmdir /s /q build
) else (
    echo [1/3] Build directory does not exist. Skipping clean.
)

REM 2. Configure CMake
echo [2/3] Configuring CMake...
cmake -S . -B build
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b %errorlevel%
)

REM 3. Build Project
echo [3/3] Building Project...
cmake --build build --config Debug
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo ==========================================
echo           Build Successful!
echo ==========================================
pause
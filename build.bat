@echo off
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
pushd "%SCRIPT_DIR%"

:: Ensure CMake and MinGW are available in PATH
where cmake.exe >nul 2>nul
if errorlevel 1 (
    set "PATH=C:\Program Files\CMake\bin;%PATH%"
)
where gcc.exe >nul 2>nul
if errorlevel 1 (
    set "PATH=C:\mingw64\bin;%PATH%"
)

echo ============================================================
echo [BUILD] Configuring CMake (MinGW Makefiles)...
echo ============================================================
cmake -B build -S . -G "MinGW Makefiles"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    popd
    exit /b %errorlevel%
)

echo.
echo ============================================================
echo [BUILD] Building test suite...
echo ============================================================
cmake --build build
if errorlevel 1 (
    echo [ERROR] Compilation failed.
    popd
    exit /b %errorlevel%
)

echo.
echo [SUCCESS] Build completed successfully.
popd
endlocal

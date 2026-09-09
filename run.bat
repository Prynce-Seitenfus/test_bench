@echo off
setlocal

:: Ensure CMake/CTest and MinGW are available in PATH
where ctest.exe >nul 2>nul
if errorlevel 1 (
    set "PATH=C:\Program Files\CMake\bin;%PATH%"
)
where gcc.exe >nul 2>nul
if errorlevel 1 (
    set "PATH=C:\mingw64\bin;%PATH%"
)

:: Build automatically if binary does not exist
if not exist "%~dp0build\test_ring_buffer.exe" (
    echo [INFO] Test binary not found. Building first...
    call "%~dp0build.bat"
    if errorlevel 1 exit /b %errorlevel%
)

echo ============================================================
echo [RUN] Running tests via CTest...
echo ============================================================
ctest --test-dir "%~dp0build" --output-on-failure
if errorlevel 1 (
    echo [ERROR] CTest reported test failures.
    exit /b %errorlevel%
)

echo.
echo ============================================================
echo [RUN] Detailed Unity Test Output:
echo ============================================================
"%~dp0build\test_ring_buffer.exe"
if errorlevel 1 (
    echo [ERROR] Unit test executable failed.
    exit /b %errorlevel%
)

echo.
echo [SUCCESS] All tests executed and passed successfully.
endlocal

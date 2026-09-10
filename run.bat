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
echo.
echo [1/7] Running Ring Buffer tests...
"%~dp0build\test_ring_buffer.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo [2/7] Running Atomic tests...
"%~dp0build\test_atomic.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo [3/7] Running Memory Pool tests...
"%~dp0build\test_memory_pool.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo [4/7] Running Linked List tests...
"%~dp0build\test_linked_list.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo [5/7] Running Bitmap tests...
"%~dp0build\test_bitmap.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo [6/7] Running CRC tests...
"%~dp0build\test_crc.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo [7/7] Running FSM tests...
"%~dp0build\test_fsm.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo ============================================================
echo [SUCCESS] All unit test suites passed successfully.
echo ============================================================
endlocal

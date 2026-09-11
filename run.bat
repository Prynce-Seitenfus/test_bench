@echo off
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
pushd "%SCRIPT_DIR%"

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
if not exist "build\test_ring_buffer.exe" (
    echo [INFO] Test binary not found. Building first...
    call "build.bat"
    if errorlevel 1 (
        popd
        exit /b %errorlevel%
    )
)

echo ============================================================
echo [RUN] Running tests via CTest...
echo ============================================================
ctest --test-dir build --output-on-failure
if errorlevel 1 (
    echo [ERROR] CTest reported test failures.
    popd
    exit /b %errorlevel%
)

echo.
echo ============================================================
echo [RUN] Detailed Unity Test Output:
echo ============================================================
echo.
echo [1/14] Running Ring Buffer tests...
build\test_ring_buffer.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [2/14] Running Atomic tests...
build\test_atomic.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [3/14] Running Memory Pool tests...
build\test_memory_pool.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [4/14] Running Linked List tests...
build\test_linked_list.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [5/14] Running Bitmap tests...
build\test_bitmap.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [6/14] Running CRC tests...
build\test_crc.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [7/14] Running FSM tests...
build\test_fsm.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [8/14] Running SertOS Task tests...
build\test_sertos_task.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [9/14] Running SertOS Scheduler tests...
build\test_sertos_scheduler.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [10/14] Running SertOS Semaphore tests...
build\test_sertos_sem.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [11/14] Running SertOS Mutex tests...
build\test_sertos_mutex.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [12/14] Running SertOS Queue tests...
build\test_sertos_queue.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [13/14] Running SertOS Timer tests...
build\test_sertos_timer.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo [14/14] Running SertOS Benchmark tests...
build\test_sertos_benchmark.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo ============================================================
echo [SUCCESS] All unit test suites passed successfully.
echo ============================================================
popd
endlocal

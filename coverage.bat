@echo off
setlocal enabledelayedexpansion

:: Ensure CMake and MinGW (gcc/gcov) are available in PATH
where cmake.exe >nul 2>nul
if errorlevel 1 (
    set "PATH=C:\Program Files\CMake\bin;%PATH%"
)
where gcc.exe >nul 2>nul
if errorlevel 1 (
    set "PATH=C:\mingw64\bin;%PATH%"
)

echo ============================================================
echo [COVERAGE] Configuring CMake with coverage enabled...
echo ============================================================
cmake -B "%~dp0build" -G "MinGW Makefiles" -DENABLE_COVERAGE=ON
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b %errorlevel%
)

echo.
echo ============================================================
echo [COVERAGE] Building test suite with coverage instrumentation...
echo ============================================================
cmake --build "%~dp0build"
if errorlevel 1 (
    echo [ERROR] Compilation failed.
    exit /b %errorlevel%
)

:: Clear stale execution counters before test run
del /s /q "%~dp0build\*.gcda" >nul 2>nul

echo.
echo ============================================================
echo [COVERAGE] Running test suite...
echo ============================================================
"%~dp0build\test_ring_buffer.exe"
if errorlevel 1 exit /b %errorlevel%
"%~dp0build\test_atomic.exe"
if errorlevel 1 exit /b %errorlevel%
"%~dp0build\test_memory_pool.exe"
if errorlevel 1 exit /b %errorlevel%
"%~dp0build\test_linked_list.exe"
if errorlevel 1 exit /b %errorlevel%
"%~dp0build\test_bitmap.exe"
if errorlevel 1 exit /b %errorlevel%
"%~dp0build\test_crc.exe"
if errorlevel 1 exit /b %errorlevel%
"%~dp0build\test_fsm.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo ============================================================
echo [COVERAGE] Generating gcov coverage reports...
echo ============================================================
if not exist "%~dp0coverage" mkdir "%~dp0coverage"
pushd "%~dp0coverage"

gcov -b -o "%~dp0build\CMakeFiles\test_ring_buffer.dir\modules\ring_buffer\ring_buffer.c.obj" "%~dp0modules\ring_buffer\ring_buffer.c"
gcov -b -o "%~dp0build\CMakeFiles\test_memory_pool.dir\modules\memory_pool\memory_pool.c.obj" "%~dp0modules\memory_pool\memory_pool.c"
gcov -b -o "%~dp0build\CMakeFiles\test_linked_list.dir\modules\linked_list\linked_list.c.obj" "%~dp0modules\linked_list\linked_list.c"
gcov -b -o "%~dp0build\CMakeFiles\test_bitmap.dir\modules\bitmap\bitmap.c.obj" "%~dp0modules\bitmap\bitmap.c"
gcov -b -o "%~dp0build\CMakeFiles\test_atomic.dir\tests\test_atomic.c.obj" "%~dp0modules\atomic\atomic.h"
gcov -b -o "%~dp0build\CMakeFiles\test_crc.dir\modules\crc\crc.c.obj" "%~dp0modules\crc\crc.c"
gcov -b -o "%~dp0build\CMakeFiles\test_fsm.dir\modules\fsm\fsm.c.obj" "%~dp0modules\fsm\fsm.c"

popd

echo.
echo ============================================================
echo [SUCCESS] Coverage analysis complete.
set "COV_DIR=%~dp0coverage\"
echo Reports generated in: !COV_DIR!
echo ============================================================
endlocal

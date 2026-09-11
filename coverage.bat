@echo off
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
pushd "%SCRIPT_DIR%"

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
cmake -B build -S . -G "MinGW Makefiles" -DENABLE_COVERAGE=ON
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    popd
    exit /b %errorlevel%
)

echo.
echo ============================================================
echo [COVERAGE] Building test suite with coverage instrumentation...
echo ============================================================
cmake --build build
if errorlevel 1 (
    echo [ERROR] Compilation failed.
    popd
    exit /b %errorlevel%
)

:: Clear stale execution counters before test run
del /s /q "build\*.gcda" >nul 2>nul

echo.
echo ============================================================
echo [COVERAGE] Running test suite...
echo ============================================================
build\test_ring_buffer.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_atomic.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_memory_pool.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_linked_list.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_bitmap.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_crc.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_fsm.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_sertos_task.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_sertos_scheduler.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_sertos_sem.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_sertos_mutex.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_sertos_queue.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )
build\test_sertos_timer.exe
if errorlevel 1 ( popd & exit /b %errorlevel% )

echo.
echo ============================================================
echo [COVERAGE] Generating gcov coverage reports...
echo ============================================================
if not exist "coverage" mkdir "coverage"
pushd "coverage"

gcov -b -o "..\build\CMakeFiles\test_ring_buffer.dir\modules\ring_buffer\ring_buffer.c.obj" "..\modules\ring_buffer\ring_buffer.c"
gcov -b -o "..\build\CMakeFiles\test_memory_pool.dir\modules\memory_pool\memory_pool.c.obj" "..\modules\memory_pool\memory_pool.c"
gcov -b -o "..\build\CMakeFiles\test_linked_list.dir\modules\linked_list\linked_list.c.obj" "..\modules\linked_list\linked_list.c"
gcov -b -o "..\build\CMakeFiles\test_bitmap.dir\modules\bitmap\bitmap.c.obj" "..\modules\bitmap\bitmap.c"
gcov -b -o "..\build\CMakeFiles\test_atomic.dir\tests\test_atomic.c.obj" "..\modules\atomic\atomic.h"
gcov -b -o "..\build\CMakeFiles\test_crc.dir\modules\crc\crc.c.obj" "..\modules\crc\crc.c"
gcov -b -o "..\build\CMakeFiles\test_fsm.dir\modules\fsm\fsm.c.obj" "..\modules\fsm\fsm.c"
gcov -b -o "..\build\CMakeFiles\test_sertos_task.dir\C_\Users\P&P\Documents\github\sertos\src\sertos_task.c.obj" "..\..\sertos\src\sertos_task.c"
gcov -b -o "..\build\CMakeFiles\test_sertos_scheduler.dir\C_\Users\P&P\Documents\github\sertos\src\sertos_scheduler.c.obj" "..\..\sertos\src\sertos_scheduler.c"
gcov -b -o "..\build\CMakeFiles\test_sertos_sem.dir\C_\Users\P&P\Documents\github\sertos\src\sertos_sem.c.obj" "..\..\sertos\src\sertos_sem.c"
gcov -b -o "..\build\CMakeFiles\test_sertos_mutex.dir\C_\Users\P&P\Documents\github\sertos\src\sertos_mutex.c.obj" "..\..\sertos\src\sertos_mutex.c"
gcov -b -o "..\build\CMakeFiles\test_sertos_queue.dir\C_\Users\P&P\Documents\github\sertos\src\sertos_queue.c.obj" "..\..\sertos\src\sertos_queue.c"
gcov -b -o "..\build\CMakeFiles\test_sertos_timer.dir\C_\Users\P&P\Documents\github\sertos\src\sertos_timer.c.obj" "..\..\sertos\src\sertos_timer.c"

popd

echo.
echo ============================================================
echo [SUCCESS] Coverage analysis complete.
echo Reports generated in: coverage\
echo ============================================================
popd
endlocal

cmake_minimum_required(VERSION 3.10)
project(RTOS_Test_Suite C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
enable_testing()

option(ENABLE_COVERAGE "Enable code coverage instrumentation (GCC/Clang)" OFF)

add_library(unity STATIC modules/Unity/src/unity.c)
target_include_directories(unity PUBLIC modules/Unity/src)

set(MODULE_INCLUDES 
    modules/atomic 
    modules/ring_buffer 
    modules/memory_pool
    modules/linked_list
    modules/bitmap
    modules/crc
    modules/fsm
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/sertos")
    set(SERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/sertos")
else()
    set(SERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sertos")
endif()

list(APPEND MODULE_INCLUDES 
    ${SERTOS_DIR}/inc 
    ${SERTOS_DIR}/port
)

set(SERTOS_SOURCES
    ${SERTOS_DIR}/src/sertos_task.c
    ${SERTOS_DIR}/src/sertos_scheduler.c
    ${SERTOS_DIR}/src/sertos_sem.c
    ${SERTOS_DIR}/src/sertos_mutex.c
    ${SERTOS_DIR}/src/sertos_queue.c
    ${SERTOS_DIR}/src/sertos_timer.c
    ${SERTOS_DIR}/port/windows/port_windows.c
    modules/ring_buffer/ring_buffer.c
    modules/linked_list/linked_list.c
    modules/bitmap/bitmap.c
    modules/memory_pool/memory_pool.c
    modules/crc/crc.c
)

# --- Test Executables ---
# 1. Ring Buffer
add_executable(test_ring_buffer tests/test_ring_buffer.c modules/ring_buffer/ring_buffer.c)
target_link_libraries(test_ring_buffer PRIVATE unity)
target_include_directories(test_ring_buffer PRIVATE ${MODULE_INCLUDES})

# 2. Atomic (Header-only)
add_executable(test_atomic tests/test_atomic.c)
target_link_libraries(test_atomic PRIVATE unity)
target_include_directories(test_atomic PRIVATE ${MODULE_INCLUDES})

# 3. Memory Pool
add_executable(test_memory_pool tests/test_memory_pool.c modules/memory_pool/memory_pool.c)
target_link_libraries(test_memory_pool PRIVATE unity)
target_include_directories(test_memory_pool PRIVATE ${MODULE_INCLUDES})

# 4. Linked List
add_executable(test_linked_list tests/test_linked_list.c modules/linked_list/linked_list.c)
target_link_libraries(test_linked_list PRIVATE unity)
target_include_directories(test_linked_list PRIVATE ${MODULE_INCLUDES})

# 5. Bitmap
add_executable(test_bitmap tests/test_bitmap.c modules/bitmap/bitmap.c)
target_link_libraries(test_bitmap PRIVATE unity)
target_include_directories(test_bitmap PRIVATE ${MODULE_INCLUDES})

# 6. CRC
add_executable(test_crc tests/test_crc.c modules/crc/crc.c)
target_link_libraries(test_crc PRIVATE unity)
target_include_directories(test_crc PRIVATE ${MODULE_INCLUDES})

# 7. FSM
add_executable(test_fsm tests/test_fsm.c modules/fsm/fsm.c)
target_link_libraries(test_fsm PRIVATE unity)
target_include_directories(test_fsm PRIVATE ${MODULE_INCLUDES})

# 8. SertOS Task Management
add_executable(test_sertos_task tests/test_sertos_task.c ${SERTOS_SOURCES})
target_link_libraries(test_sertos_task PRIVATE unity)
target_include_directories(test_sertos_task PRIVATE ${MODULE_INCLUDES})

# 9. SertOS Preemptive Scheduler Core
add_executable(test_sertos_scheduler tests/test_sertos_scheduler.c ${SERTOS_SOURCES})
target_link_libraries(test_sertos_scheduler PRIVATE unity)
target_include_directories(test_sertos_scheduler PRIVATE ${MODULE_INCLUDES})

# 10. SertOS Semaphore Primitive
add_executable(test_sertos_sem tests/test_sertos_sem.c ${SERTOS_SOURCES})
target_link_libraries(test_sertos_sem PRIVATE unity)
target_include_directories(test_sertos_sem PRIVATE ${MODULE_INCLUDES})

# 11. SertOS Mutex (PIP) Primitive
add_executable(test_sertos_mutex tests/test_sertos_mutex.c ${SERTOS_SOURCES})
target_link_libraries(test_sertos_mutex PRIVATE unity)
target_include_directories(test_sertos_mutex PRIVATE ${MODULE_INCLUDES})

# 12. SertOS Message Queue Primitive
add_executable(test_sertos_queue tests/test_sertos_queue.c ${SERTOS_SOURCES})
target_link_libraries(test_sertos_queue PRIVATE unity)
target_include_directories(test_sertos_queue PRIVATE ${MODULE_INCLUDES})

# 13. SertOS Software Timers Primitive
add_executable(test_sertos_timer tests/test_sertos_timer.c ${SERTOS_SOURCES})
target_link_libraries(test_sertos_timer PRIVATE unity)
target_include_directories(test_sertos_timer PRIVATE ${MODULE_INCLUDES})

# 14. SertOS Execution Benchmarks (Latency, Jitter, Footprint)
add_executable(test_sertos_benchmark tests/test_sertos_benchmark.c ${SERTOS_SOURCES})
target_link_libraries(test_sertos_benchmark PRIVATE unity)
target_include_directories(test_sertos_benchmark PRIVATE ${MODULE_INCLUDES})

set(ALL_TEST_TARGETS 
    test_ring_buffer 
    test_atomic 
    test_memory_pool 
    test_linked_list 
    test_bitmap 
    test_crc 
    test_fsm
    test_sertos_task
    test_sertos_scheduler
    test_sertos_sem
    test_sertos_mutex
    test_sertos_queue
    test_sertos_timer
    test_sertos_benchmark
)

foreach(TARGET ${ALL_TEST_TARGETS})
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra -pedantic)
    if(ENABLE_COVERAGE)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${TARGET} PRIVATE -fprofile-arcs -ftest-coverage -O0)
            target_link_options(${TARGET} PRIVATE -fprofile-arcs -ftest-coverage)
        else()
            message(WARNING "Coverage requested, but compiler is not GNU or Clang.")
        endif()
    endif()
endforeach()

# --- Cyclomatic Complexity & Maintainability Gate (Lizard) ---
option(ENABLE_LIZARD_GATE "Enforce Cyclomatic Complexity Gate (CCN <= 10, NLOC <= 75, Params <= 5)" ON)

if(ENABLE_LIZARD_GATE)
    find_program(
        LIZARD_BIN
        NAMES lizard lizard.exe
        HINTS
            "$ENV{LOCALAPPDATA}/Python/pythoncore-3.14-64/Scripts"
            "$ENV{LOCALAPPDATA}/Programs/Python/Python*/Scripts"
            "$ENV{APPDATA}/Python/Scripts"
    )

    if(NOT LIZARD_BIN)
        find_program(PYTHON_BIN NAMES python3 python py)
        if(PYTHON_BIN)
            execute_process(
                COMMAND ${PYTHON_BIN} -m lizard --version
                RESULT_VARIABLE LIZARD_CHECK_RESULT
                OUTPUT_QUIET ERROR_QUIET
            )
            if(LIZARD_CHECK_RESULT EQUAL 0)
                set(LIZARD_BIN ${PYTHON_BIN} -m lizard)
            endif()
        endif()
    endif()

    if(NOT LIZARD_BIN)
        message(FATAL_ERROR "Lizard code analyzer not found! Install via: pip install lizard")
    endif()

    set(COMPLEXITY_SCAN_DIRS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
        list(APPEND COMPLEXITY_SCAN_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules")
        list(APPEND COMPLEXITY_SCAN_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/modules")
    endif()
    if(EXISTS "${SERTOS_DIR}/src")
        list(APPEND COMPLEXITY_SCAN_DIRS "${SERTOS_DIR}/src")
    endif()
    if(EXISTS "${SERTOS_DIR}/port")
        list(APPEND COMPLEXITY_SCAN_DIRS "${SERTOS_DIR}/port")
    endif()
    if(NOT COMPLEXITY_SCAN_DIRS)
        set(COMPLEXITY_SCAN_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    add_custom_target(
        check_complexity
        COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_CURRENT_SOURCE_DIR}"
                ${LIZARD_BIN}
                -C 10
                -L 75
                -a 5
                -w
                -W whitelizard.txt
                -x "*/Unity/*"
                -x "*/build/*"
                -x "*/CMakeFiles/*"
                ${COMPLEXITY_SCAN_DIRS}
        COMMENT "Enforcing RTOS Quality Metrics (Max CCN: 10 | Max NLOC: 75 | Max Params: 5)..."
        VERBATIM
    )

    foreach(TARGET ${ALL_TEST_TARGETS})
        add_dependencies(${TARGET} check_complexity)
    endforeach()
endif()

add_test(NAME Validacao_RingBuffer COMMAND test_ring_buffer)
add_test(NAME Validacao_Atomic COMMAND test_atomic)
add_test(NAME Validacao_MemoryPool COMMAND test_memory_pool)
add_test(NAME Validacao_LinkedList COMMAND test_linked_list)
add_test(NAME Validacao_Bitmap COMMAND test_bitmap)
add_test(NAME Validacao_CRC COMMAND test_crc)
add_test(NAME Validacao_FSM COMMAND test_fsm)
add_test(NAME Validacao_SertOSTask COMMAND test_sertos_task)
add_test(NAME Validacao_SertOSScheduler COMMAND test_sertos_scheduler)
add_test(NAME Validacao_SertOSSem COMMAND test_sertos_sem)
add_test(NAME Validacao_SertOSMutex COMMAND test_sertos_mutex)
add_test(NAME Validacao_SertOSQueue COMMAND test_sertos_queue)
add_test(NAME Validacao_SertOSTimer COMMAND test_sertos_timer)
add_test(NAME Validacao_SertOSBenchmark COMMAND test_sertos_benchmark)
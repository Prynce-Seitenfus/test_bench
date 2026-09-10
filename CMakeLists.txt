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

set(ALL_TEST_TARGETS test_ring_buffer test_atomic test_memory_pool test_linked_list test_bitmap test_crc test_fsm)

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
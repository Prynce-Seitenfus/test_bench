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

add_test(NAME Validacao_RingBuffer COMMAND test_ring_buffer)
add_test(NAME Validacao_Atomic COMMAND test_atomic)
add_test(NAME Validacao_MemoryPool COMMAND test_memory_pool)
add_test(NAME Validacao_LinkedList COMMAND test_linked_list)
add_test(NAME Validacao_Bitmap COMMAND test_bitmap)
add_test(NAME Validacao_CRC COMMAND test_crc)
add_test(NAME Validacao_FSM COMMAND test_fsm)
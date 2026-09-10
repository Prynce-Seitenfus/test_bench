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
)

add_executable(test_ring_buffer tests/test_ring_buffer.c modules/ring_buffer/ring_buffer.c)
target_link_libraries(test_ring_buffer PRIVATE unity)
target_include_directories(test_ring_buffer PRIVATE ${MODULE_INCLUDES})

if(ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_ring_buffer PRIVATE -fprofile-arcs -ftest-coverage -O0 -Wall -Wextra)
        target_link_options(test_ring_buffer PRIVATE -fprofile-arcs -ftest-coverage)
    else()
        message(WARNING "Coverage requested, but compiler is not GNU or Clang.")
    endif()
endif()

add_test(NAME Validacao_RingBuffer COMMAND test_ring_buffer)
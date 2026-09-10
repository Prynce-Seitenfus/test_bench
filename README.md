# RTOS Centralized Test Suite

A centralized, automated unit testing harness for embedded C modules powered by the **Unity** test framework, with CMake, MinGW Makefiles, and automated code coverage analysis.

---

## Project Structure

```text
test_bench/
├── CMakeLists.txt              # Primary CMake build configuration
├── Makefile                    # Alternative MinGW Makefile
├── build.bat                   # 1-click CMake build script for Windows
├── run.bat                     # 1-click test execution script (CTest + Unity)
├── coverage.bat                # 1-click gcov / lcov code coverage script
├── modules/                    # Submodule dependencies under test
│   ├── Unity/                  # Unity test framework
│   ├── atomic/                 # Atomic abstraction layer
│   ├── ring_buffer/            # Lock-free SPSC circular buffer
│   ├── memory_pool/            # Deterministic static block allocator
│   ├── linked_list/            # Intrusive circular doubly linked list
│   ├── bitmap/                 # Bit array & CLZ/CTZ priority lookup
│   ├── crc/                    # Hardware/table-driven CRC-8/16/32
│   └── fsm/                    # Table-driven Finite State Machine
└── tests/                      # Centralized test suites
    ├── test_atomic.c           # Memory barriers & atomic operations
    ├── test_ring_buffer.c      # SPSC FIFO & wrap-around
    ├── test_memory_pool.c      # Static allocation, splitting & coalescing
    ├── test_linked_list.c      # Intrusive list O(1) & direct operations
    ├── test_bitmap.c           # O(1) priority search & bitwise ops
    ├── test_crc.c              # CRC-8/16/32 standard test vectors
    └── test_fsm.c              # State transitions, guards & lifecycle hooks
```

---

## Test Suites Overview

| Module | Test File | Key Test Coverage |
| :--- | :--- | :--- |
| **`atomic`** | `tests/test_atomic.c` | Release/acquire barriers, relaxed loads/stores, standalone fences, thread safety semantics. |
| **`ring_buffer`** | `tests/test_ring_buffer.c` | Power-of-2 capacity validation, SPSC FIFO ordering, full/empty boundaries, 1500+ wrap-arounds, bulk `write`/`read`. |
| **`memory_pool`** | `tests/test_memory_pool.c` | First-fit allocation, 8-byte alignment, block splitting, adjacent free coalescing, `calloc` overflow defense, `realloc`. |
| **`linked_list`** | `tests/test_linked_list.c` | Sentinel node integrity, deterministic $O(1)$ inlined `pop`/`peek`, direct primitives, before/after insertion, duplicate link prevention. |
| **`bitmap`** | `tests/test_bitmap.c` | Single-bit manipulation, multi-word boundary masking, hardware $O(1)$ CLZ/CTZ priority discovery, unified forward/backward scanners. |
| **`crc`** | `tests/test_crc.c` | Standard test vectors (`"123456789"`), streaming accumulation vs single-shot inlined wrappers, `NULL` and zero-length safety. |
| **`fsm`** | `tests/test_fsm.c` | Table-driven state transitions, boolean guard rejections, `on_enter`/`on_exit` hooks, self-transitions, invalid event defenses. |

---

## How to Build and Run Tests

### Option 1: Quick Batch Scripts (Windows)

To build:
```cmd
build.bat
```

To run all 7 test suites:
```cmd
run.bat
```

To run with gcov code coverage reporting:
```cmd
coverage.bat
```

### Option 2: CMake & CTest

```powershell
cmake -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

### Option 3: Make

```powershell
mingw32-make test
```

---

## Verification & Code Coverage

All 7 test suites achieve **100% pass rate** (68/68 test cases passing) and comprehensive statement and branch coverage:
* `crc.c`: 100%
* `fsm.c`: 98.4%
* `linked_list.c`: 96.9%
* `memory_pool.c`: 94.9%
* `bitmap.c`: 90.3%
* `ring_buffer.c`: 86.7%

---

## Standards Compliance

* **Freestanding ISO C99**: Zero dynamic memory allocation (`malloc`/`free`) per MISRA C:2012 Rule 21.3.
* **Workspace Guidelines**: 100% compliant with [`GEMINI.md`](../GEMINI.md) (1TBS braces, 4-space indent, `/* ... */` block comments exclusively, unsigned literals with `U` suffix).

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
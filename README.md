# RTOS Centralized Test Suite

A centralized, automated unit testing harness for embedded C modules powered by the **Unity** test framework, with CMake, MinGW Makefiles, and automated code coverage analysis.

---

## Project Structure

```text
test_bench/
├── CMakeLists.txt              # Primary CMake build configuration
├── Makefile                    # Alternative MinGW Makefile
├── build.bat                   # 1-click CMake build script & complexity gate
├── run.bat                     # 1-click test execution script (CTest + Unity)
├── coverage.bat                # 1-click gcov / lcov code coverage script
├── lizard.bat                  # 1-click standalone complexity & quality gate script
├── whitelizard.txt             # Whitelist for complexity thresholds
├── modules/                    # Submodule dependencies under test
│   ├── Unity/                  # Unity test framework
│   ├── atomic/                 # Atomic abstraction layer
│   ├── ring_buffer/            # Lock-free SPSC circular buffer
│   ├── memory_pool/            # Deterministic static block allocator
│   ├── linked_list/            # Intrusive circular doubly linked list
│   ├── bitmap/                 # Bit array & CLZ/CTZ priority lookup
│   ├── crc/                    # Hardware/table-driven CRC-8/16/32
│   └── fsm/                    # Table-driven Finite State Machine
└── tests/                      # Centralized test suites (14 suites)
    ├── test_atomic.c           # Memory barriers & atomic operations
    ├── test_ring_buffer.c      # SPSC FIFO & wrap-around
    ├── test_memory_pool.c      # Static allocation, splitting & coalescing
    ├── test_linked_list.c      # Intrusive list O(1) & direct operations
    ├── test_bitmap.c           # O(1) priority search & bitwise ops
    ├── test_crc.c              # CRC-8/16/32 standard test vectors
    ├── test_fsm.c              # State transitions, guards & lifecycle hooks
    ├── test_sertos_task.c      # Task lifecycle, TCB CRC & stack high-water
    ├── test_sertos_scheduler.c # O(1) preemptive scheduling & round-robin
    ├── test_sertos_sem.c       # Binary & counting semaphores with ISR signaling
    ├── test_sertos_mutex.c     # PIP mutex, recursion & anti-priority inversion
    ├── test_sertos_queue.c     # Multi-task FIFO message queue (ring_buffer)
    ├── test_sertos_timer.c     # Monotonic software timers (one-shot & periodic)
    └── test_sertos_benchmark.c # Context switch latency & O(1) scaling benchmarks
```

---

## Test Suites Overview

| Module / Component | Test File | Key Test Coverage |
| :--- | :--- | :--- |
| **`atomic`** | `tests/test_atomic.c` | Release/acquire barriers, relaxed loads/stores, standalone fences, thread safety semantics. |
| **`ring_buffer`** | `tests/test_ring_buffer.c` | Power-of-2 capacity validation, SPSC FIFO ordering, full/empty boundaries, wrap-around, bulk IO. |
| **`memory_pool`** | `tests/test_memory_pool.c` | First-fit allocation, 8-byte alignment, block splitting, adjacent free coalescing, `realloc`. |
| **`linked_list`** | `tests/test_linked_list.c` | Sentinel node integrity, deterministic $O(1)$ inlined `pop`/`peek`, direct primitives, before/after insertion. |
| **`bitmap`** | `tests/test_bitmap.c` | Single-bit manipulation, multi-word boundary masking, hardware $O(1)$ CLZ/CTZ priority discovery. |
| **`crc`** | `tests/test_crc.c` | Standard test vectors (`"123456789"`), streaming accumulation vs single-shot inlined wrappers. |
| **`fsm`** | `tests/test_fsm.c` | Table-driven state transitions, boolean guard rejections, `on_enter`/`on_exit` hooks, lifecycle defenses. |
| **`sertos_task`** | `tests/test_sertos_task.c` | Task creation (static/dynamic), stack painting (`0xA5`), high-water mark, TCB CRC protection, suspend/resume. |
| **`sertos_scheduler`** | `tests/test_sertos_scheduler.c` | $O(1)$ bitmap priority selection, round-robin time slicing, critical section nesting, delay queue wakeup. |
| **`sertos_sem`** | `tests/test_sertos_sem.c` | Counting & binary semaphores, unblocking highest priority, ISR take/give signaling without blocking. |
| **`sertos_mutex`** | `tests/test_sertos_mutex.c` | Priority Inheritance Protocol (PIP), recursive re-entrancy, ownership validation, chained elevation restore. |
| **`sertos_queue`** | `tests/test_sertos_queue.c` | Multi-task thread-safe FIFO message queue backed by `ring_buffer`, timeout blocking, ISR enqueue/dequeue. |
| **`sertos_timer`** | `tests/test_sertos_timer.c` | Monotonic software timers (one-shot & periodic), period updates, tick dispatching. |
| **`sertos_benchmark`** | `tests/test_sertos_benchmark.c` | Context switch latency, $O(1)$ schedule time independence from task count, PIP overhead, FIFO throughput. |

---

## How to Build and Run Tests

### Option 1: Quick Batch Scripts (Windows)

To build (with automated Lizard complexity gating):
```cmd
build.bat
```

To run all 14 test suites:
```cmd
run.bat
```

To run with gcov code coverage reporting:
```cmd
coverage.bat
```

To run standalone Lizard code complexity analysis:
```cmd
lizard.bat
# Or warnings-only:
lizard.bat -w
```

### Option 2: CMake & CTest

```powershell
cmake -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Verification & Code Coverage

All 14 test suites achieve a **100% pass rate** (114+ test assertions) across both foundational modules and kernel components:
* `crc.c`: 100%
* `fsm.c`: 98.3%
* `linked_list.c`: 97.8%
* `memory_pool.c`: 95.6%
* `bitmap.c`: 92.3%
* `ring_buffer.c`: 87.7%
* `sertos_task.c`: 88.9%
* `sertos_timer.c`: 84.9%
* `sertos_mutex.c`: 76.4%
* `sertos_sem.c`: 75.0%
* `sertos_queue.c`: 70.4%
* `sertos_scheduler.c`: 65.0%

### Complexity Gate (Lizard)
- 27 files analyzed, 212 functions analyzed.
- Average CCN: 3.4, Average NLOC: 12.9.
- **0 threshold violations** (`CCN <= 10`, `NLOC <= 75`, `Params <= 5`).

---

## Standards Compliance

* **Freestanding ISO C99**: Zero dynamic memory allocation (`malloc`/`free`) per MISRA C:2012 Rule 21.3.
* **Workspace Guidelines**: 100% compliant with [`GEMINI.md`](../GEMINI.md) (1TBS braces, 4-space indent, `/* ... */` block comments exclusively, unsigned literals with `U` suffix).

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
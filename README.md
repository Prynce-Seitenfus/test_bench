# RTOS Test Suite

A centralized, automated unit testing harness for embedded C modules (`atomic`, `ring_buffer`, `memory_pool`) powered by the **Unity** test framework.

---

## Project Structure

```text
test/
├── CMakeLists.txt        # Primary CMake configuration
├── Makefile              # MinGW Makefile alternative
├── build.bat             # 1-click CMake build script for Windows
├── run.bat               # 1-click test execution script (CTest + Unity)
├── modules/              # Submodules for libraries under test
│   ├── Unity/            # Unity test framework
│   ├── atomic/           # Atomic abstraction layer
│   ├── memory_pool/      # Static memory allocator
│   └── ring_buffer/      # Lock-free circular buffer
└── tests/                # Centralized test suites
    └── test_ring_buffer.c
```

---

## Test Suites

### 1. Ring Buffer (`tests/test_ring_buffer.c`)
* **Initialization**: Validates power-of-two capacities and rejects non-power-of-two sizes and `NULL` pointers.
* **Core FIFO Logic**: Confirms sequential data ordering across single and multi-byte transfers.
* **Boundary Conditions**: Tests full-buffer reject behavior and empty-buffer pop defense.
* **Index Wrap-Around**: Executes 1,500+ push/pop operations across boundary transitions to prove bitwise masking correctness.

---

## How to Build and Run Tests

### Option 1: Quick Batch Scripts (Windows)

To build:
```cmd
build.bat
```

To run tests:
```cmd
run.bat
```

To run tests with code coverage analysis:
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

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic
INCLUDES = -Imodules/Unity/src -Imodules/atomic -Imodules/ring_buffer -Imodules/memory_pool -Imodules/linked_list -Imodules/bitmap -Imodules/crc -Imodules/fsm

UNITY_SRC = modules/Unity/src/unity.c

TARGETS = test_ring_buffer.exe test_atomic.exe test_memory_pool.exe test_linked_list.exe test_bitmap.exe test_crc.exe test_fsm.exe

all: $(TARGETS)

test_ring_buffer.exe: $(UNITY_SRC) modules/ring_buffer/ring_buffer.c tests/test_ring_buffer.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

test_atomic.exe: $(UNITY_SRC) tests/test_atomic.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

test_memory_pool.exe: $(UNITY_SRC) modules/memory_pool/memory_pool.c tests/test_memory_pool.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

test_linked_list.exe: $(UNITY_SRC) modules/linked_list/linked_list.c tests/test_linked_list.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

test_bitmap.exe: $(UNITY_SRC) modules/bitmap/bitmap.c tests/test_bitmap.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

test_crc.exe: $(UNITY_SRC) modules/crc/crc.c tests/test_crc.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

test_fsm.exe: $(UNITY_SRC) modules/fsm/fsm.c tests/test_fsm.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

test: all
	./test_ring_buffer.exe
	./test_atomic.exe
	./test_memory_pool.exe
	./test_linked_list.exe
	./test_bitmap.exe
	./test_crc.exe
	./test_fsm.exe

clean:
	-del /Q $(TARGETS) 2>nul

.PHONY: all test clean

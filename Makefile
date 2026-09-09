CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic
INCLUDES = -Imodules/Unity/src -Imodules/atomic -Imodules/ring_buffer -Imodules/memory_pool

SRCS = modules/Unity/src/unity.c \
       modules/ring_buffer/ring_buffer.c \
       tests/test_ring_buffer.c

TARGET = test_ring_buffer.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	-del /Q $(TARGET) 2>nul

.PHONY: all test clean

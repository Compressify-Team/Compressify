CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =

# Source files
SRCS = src/main.c src/arith_cod.c

# Object files
OBJS = $(patsubst src/%.c, obj/%.o, $(SRCS))

# Executable name
TARGET = bin/compressify

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compile source files to object files
obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

# Clean up build files
clean:
	rm -f obj/*.o bin/$(TARGET)

# Phony targets
.PHONY: all clean run
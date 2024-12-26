CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =

# Source files
SRCS = src/main.c src/arith_cod.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = bin/compressify

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)


# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
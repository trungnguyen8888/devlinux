# Toolchain configuration
CC := gcc
AR := ar
CFLAGS := -Wall -Wextra -pedantic -Werror -std=c99

# Module and path configuration
MODULE := ipv4_parser
INC := -Iinclude
SRC := $(wildcard src/*.c)

# Build artifacts
BUILD_DIR := build
OBJ := $(SRC:src/%.c=$(BUILD_DIR)/%.o)
TARGET := lib/lib$(MODULE).a

# Phony targets are commands, not generated files.
.PHONY: all clean

# Default target: build the IPv4 parser static library.
all: $(TARGET)

# Archive all object files into a static library.
$(TARGET): $(OBJ)
	mkdir -p lib
	$(AR) rcs $(TARGET) $(OBJ)
	rm -f $(OBJ)

# Compile each source file into the build directory.
$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(INC) $(CFLAGS) -c $< -o $@

# Remove all generated build outputs.
clean:
	rm -rf $(BUILD_DIR) lib
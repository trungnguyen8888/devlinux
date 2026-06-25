# Compiler and build flags
CC := gcc
CFLAGS := -Wall -Wextra -pedantic -Werror -std=c99

# Library module information
MODULE := ipv4_parser
INC := -I../../include
LIBDIR := ../../lib
LIBTARGET := $(LIBDIR)/lib$(MODULE).a
LIB := -L$(LIBDIR) -l$(MODULE)

# Integration test source and output
SRC := main.c
BUILD_DIR := ../../build
OBJ := $(SRC:.c=.o)
TARGET := $(BUILD_DIR)/integration_test

# These targets are commands, not real files.
.PHONY: all clean

# Default target: build the integration test executable.
all: $(TARGET)

# Link the integration test with the IPv4 parser static library.
$(TARGET): $(OBJ) $(LIBTARGET)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIB)
	rm -f $(OBJ)

# Build the library from the project root if it does not exist.
$(LIBTARGET):
	$(MAKE) -C ../.. all

# Compile the integration test source file.
%.o: %.c
	$(CC) $(INC) $(CFLAGS) -c $< -o $@

# Remove integration test outputs and clean the library build.
clean:
	rm -f $(OBJ) $(TARGET)
	$(MAKE) -C ../.. clean
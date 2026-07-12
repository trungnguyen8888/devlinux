CC := gcc
CFLAGS := -Wall -Wextra -pedantic -Werror -std=c99
TARGET := mem_pool
SRC := main.c packet_util.c
HDR := packet_util.h
INC := -I.

.PHONY: all clean run clang_tidy cpp_check size

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(INC) $(SRC) -o $(TARGET)

run: $(TARGET)
	$(shell pwd)/$<

clang_tidy: $(SRC) $(HDR)
	clang-tidy $^ -checks='clang-analyzer-*,cert-*,bugprone-*' -- -std=c99

cpp_check: $(SRC)
	cppcheck --enable=all --inconclusive --std=c99 --language=c $(INC) --suppress=missingIncludeSystem $^

size: $(TARGET)
	@echo "Size of $<"
	size $<

clean:
	rm -f $(TARGET)
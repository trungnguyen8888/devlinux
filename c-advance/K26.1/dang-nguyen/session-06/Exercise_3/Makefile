CC := gcc
CFLAGS := -Wall -Wextra -pedantic -Werror -std=c99
TARGET := cmd_dispatcher
SRC := main.c cmd_dispatcher.c
HDR := cmd_dispatcher.h
INC := -I.

.PHONY: all clean run clang_tidy cpp_check

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(INC) $(SRC) -o $(TARGET)

run: $(TARGET)
	$(shell pwd)/$<

clang_tidy: $(SRC) $(HDR)
	clang-tidy $^ -checks='clang-analyzer-*,cert-*,bugprone-*' -- -std=c99

cpp_check: $(SRC)
	cppcheck --enable=all --inconclusive --std=c99 --language=c $(INC) --suppress=missingIncludeSystem $^

clean:
	rm -f $(TARGET)
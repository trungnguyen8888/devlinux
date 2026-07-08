CC := gcc
CFLAGS := -Wall -Wextra -pedantic -Werror -std=c99
TARGET := ex_4
SRC := main.c
INC := -I.

.PHONY: all clean run clang_tidy cpp_check

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INC) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clang_tidy: $(SRC)
	clang-tidy $< -checks='clang-analyzer-*,cert-*,bugprone-*' -- -std=c99 $(INC)

cpp_check: $(SRC)
	cppcheck --enable=all --inconclusive --std=c99 --language=c \
		--suppress=missingIncludeSystem \
		--suppress=unusedStructMember:$(SRC) \
		$<

clean:
	rm -f $(TARGET)
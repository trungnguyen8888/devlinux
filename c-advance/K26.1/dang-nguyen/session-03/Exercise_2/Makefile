CC := gcc
CFLAGS := -Wall -Wextra -pedantic -Werror -std=c99
TARGET := struct_size
SRC := main.c
INC := -I.

.PHONY: all clean run pointer-warning-demo clang_tidy cpp_check

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INC) $(SRC) -o $(TARGET)

run: all
	./$(TARGET)

pointer-warning-demo: $(SRC)
	$(CC) $(CFLAGS) -Wno-error=address-of-packed-member -DENABLE_UNSAFE_POINTER_DEMO $(INC) $(SRC) -o $(TARGET)

clang_tidy: $(SRC)
	clang-tidy $< -checks='clang-analyzer-*,cert-*,bugprone-*' -- -std=c99 $(INC)

cpp_check: $(SRC)
	cppcheck --enable=all --inconclusive --std=c99 --language=c \
		--suppress=missingIncludeSystem \
		--suppress=unusedStructMember:$(SRC) \
		$<

clean:
	rm -f $(TARGET)
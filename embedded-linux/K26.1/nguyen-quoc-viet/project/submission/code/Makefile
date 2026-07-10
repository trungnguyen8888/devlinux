CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lcrypto
TARGET = chat-server

SRCS = main.c auth.c message.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "✅ Build successful: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "🧹 Cleaned"

.PHONY: all clean

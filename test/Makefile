# Test Makefile (pure C)

CC = gcc

CFLAGS = -Wall -Wextra -g \
	-I. \
	-Iinclude \
	-I.. \
	-DDISABLE_NETWORK

LDFLAGS =
LIBS = -luuid -lssl -lcrypto

TEST_SRCS = \
	test_main.c \
	test_framework.c

TEST_OBJS = $(TEST_SRCS:.c=.o) account_app.o server_api_app.o ui_app.o

TARGET = test_runner

all: $(TARGET)

$(TARGET): $(TEST_OBJS)
	$(CC) $(LDFLAGS) $(TEST_OBJS) $(LIBS) -o $(TARGET)

account_app.o: ../account.c
	$(CC) $(CFLAGS) -c $< -o $@

server_api_app.o: ../server_api.c
	$(CC) $(CFLAGS) -c $< -o $@

ui_app.o: ../ui.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

test: run

clean:
	rm -f $(TEST_OBJS) $(TARGET)

.PHONY: all run test clean

CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS=

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

TEST_BIN=tests/test_proc_parser
TEST_SRC=tests/test_proc_parser.c src/proc_parser.c
TEST_OBJ=$(TEST_SRC:.c=.o)

all: system_pulse

system_pulse: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(TEST_BIN): $(TEST_OBJ)
	$(CC) $(TEST_OBJ) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJ) $(TEST_OBJ) system_pulse $(TEST_BIN)

.PHONY: all clean test

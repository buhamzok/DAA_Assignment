CC = gcc
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Isrc -I../modules/common
LDFLAGS = -lm
SRC = src/server.c src/http.c src/json.c \
      ../modules/common/minijson.c ../modules/datastructures.c \
      ../modules/recursion.c ../modules/sorting.c ../modules/benchmark.c
OBJ = $(SRC:.c=.o)
TARGET = algorithm_explorer

ifeq ($(OS),Windows_NT)
    TARGET = algorithm_explorer.exe
    LDFLAGS += -lws2_32
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c src/*.h
	$(CC) $(CFLAGS) -c $< -o $@

../modules/%.o: ../modules/%.c
	$(CC) $(CFLAGS) -c $< -o $@

../modules/common/%.o: ../modules/common/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	$(TARGET)

test:
	$(CC) $(CFLAGS) -I./src tests/test_algorithms.c ../modules/common/minijson.c ../modules/datastructures.c ../modules/recursion.c ../modules/sorting.c src/json.c -o algorithm_tests $(LDFLAGS)
	algorithm_tests.exe

clean:
ifeq ($(OS),Windows_NT)
	-powershell -NoProfile -Command "Remove-Item -Force -ErrorAction SilentlyContinue src/server.o,src/http.o,src/json.o,../modules/common/minijson.o,../modules/datastructures.o,../modules/recursion.o,../modules/sorting.o,../modules/benchmark.o,$(TARGET),algorithm_tests.exe; exit 0"
else
	rm -f $(OBJ) $(TARGET)
endif

.PHONY: all run test clean

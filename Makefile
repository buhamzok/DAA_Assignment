CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic
LDFLAGS =
SRC = src/server.c src/http.c src/json.c
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

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all run clean

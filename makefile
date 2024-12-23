CC = gcc
CFLAGS = 
OBJ = obj/main.o
BIN = bin/my_test_project

all: $(BIN)

obj/%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

run: $(BIN)
	./$(BIN)

.PHONY: clean

clean:
	rm -f obj/*.o bin/*
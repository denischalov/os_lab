CC = gcc
CFLAGS = -I$(INCLUDE)

SRC = src
INCLUDE = include
BIN = bin
OBJ = obj

.PHONY: all clean

all: $(BIN)/parallel_min_max $(BIN)/zombie_process

$(BIN)/parallel_min_max: $(OBJ)/parallel_min_max.o $(OBJ)/utils.o $(OBJ)/find_min_max.o | $(BIN)
	$(CC) -o $@ $^

$(BIN)/zombie_process: $(SRC)/zombie_process.c
	$(CC) -o $@ $^

$(OBJ)/parallel_min_max.o: $(SRC)/parallel_min_max.c $(INCLUDE)/utils.h $(INCLUDE)/find_min_max.h | $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ)/utils.o: $(SRC)/utils.c $(INCLUDE)/utils.h | $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ)/find_min_max.o: $(SRC)/find_min_max.c $(INCLUDE)/utils.h $(INCLUDE)/find_min_max.h | $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN) $(OBJ):
	mkdir -p $@

clean:
	rm -rf $(BIN) $(OBJ)
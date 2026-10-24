CC=gcc
CFLAGS=-I.
BIN=bin
OBJ=obj

all: $(BIN)/sequential_min_max $(BIN)/parallel_min_max $(BIN)/exec

$(BIN) $(OBJ):
	mkdir -p $@

$(BIN)/sequential_min_max : $(OBJ)/utils.o $(OBJ)/find_min_max.o utils.h find_min_max.h $(BIN)
	$(CC) -o $@ $(OBJ)/find_min_max.o $(OBJ)/utils.o sequential_min_max.c $(CFLAGS)

$(BIN)/parallel_min_max : $(OBJ)/utils.o $(OBJ)/find_min_max.o utils.h find_min_max.h $(BIN)
	$(CC) -o $@ $(OBJ)/utils.o $(OBJ)/find_min_max.o parallel_min_max.c $(CFLAGS)

$(BIN)/exec : exec.c $(BIN)
	$(CC) -o $@ exec.c $(CFLAGS)

$(OBJ)/utils.o : utils.c utils.h $(OBJ)
	$(CC) -o $@ -c utils.c $(CFLAGS)

$(OBJ)/find_min_max.o : find_min_max.c utils.h find_min_max.h $(OBJ)
	$(CC) -o $@ -c find_min_max.c $(CFLAGS)

clean :
	rm -rf $(BIN) $(OBJ)
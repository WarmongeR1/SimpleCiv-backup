# object files to compile
OBJ = int_array.o dyn_array.o string_functions.o test.o

# debug flag
DEBUG = -g

# common flags
FLAGS = -std=c99 -Wall -pedantic

# flags for compilation object files
CFLAGS = $(FLAGS) -c $(DEBUG)

# flags for linking
LFLAGS = $(FLAGS) $(DEBUG)

test: $(OBJ)
	gcc $(LFLAGS) -o test $(OBJ)

int_array.o: ../../modules/int_array/int_array.c
	gcc $(CFLAGS) ../../modules/int_array/int_array.c

dyn_array.o: ../../modules/dyn_array/dyn_array.c
	gcc $(CFLAGS) ../../modules/dyn_array/dyn_array.c

string_functions.o: ../../modules/parser/string_functions.c
	gcc $(CFLAGS) ../../modules/parser/string_functions.c

test.o: test.c
	gcc $(CFLAGS) test.c

clean:
	rm -f *.o test

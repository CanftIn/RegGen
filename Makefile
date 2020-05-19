CFLAGS = -std=c11 -g -Wall -Wextra
CC = gcc

#all: test_maxargs test_interp test_persistent_tree test_persistent_binding_tree

prog1.o: prog1.c slp.h util.h
	$(CC) $(CFLAGS) -c prog1.c

slp.o: slp.c slp.h util.h
	$(CC) $(CFLAGS) -c slp.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

.PHONY: clean
clean: 
	rm -f a.out *.o \
	test_maxargs test_interp test_persistent_tree test_persistent_binding_tree
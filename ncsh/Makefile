CC = gcc
CFLAGS = -Wall -g -O0 -Wwrite-strings \
-pedantic-errors -fstack-protector-all -Wextra
LDFLAGS = -lreadline
DEBUGFLAGS = -D DEBUG_MODE
PROGS = ncsh

.PHONY: all clean

all: $(PROGS)

ifeq ($(DEBUG), y)
parser.o: parser.c scanner.h tree.h process_tree.h executor.h globexp.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -c parser.c

process_tree.o: process_tree.c process_tree.h tree.h
endif

ncsh: ncsh.o parser.o scanner.o executor.o process_tree.o globexp.o
	$(CC) -o ncsh ncsh.o parser.o scanner.o executor.o process_tree.o globexp.o $(LDFLAGS) 

ncsh.o: ncsh.c tree.h
parser.o: parser.c scanner.h tree.h process_tree.h executor.h globexp.h
scanner.o: scanner.c scanner.h
executor.o: executor.c executor.h tree.h
globexp.o: globexp.c tinydir.h

clean:
	@rm -f $(PROGS) *.o

CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -g -I/opt/homebrew/include
DEPS = server.h request.h constants.h mime.h list.h hashmap.h
OBJ = srv.o server.o request.o mime.o list.o hashmap.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

srvd: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ) srv

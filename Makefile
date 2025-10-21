CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
CPPFLAGS ?=
LDFLAGS ?=
DEPS = server.h request.h constants.h mime.h list.h hashmap.h
OBJ = srv.o server.o request.o mime.o list.o hashmap.o
RM = rm -f

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

srvd: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	$(RM) $(OBJ) srvd
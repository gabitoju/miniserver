CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=gnu17 -g
CPPFLAGS ?=
LDFLAGS ?=
DEPS = server.h request.h constants.h mime.h list.h hashmap.h
OBJ = srv.o server.o request.o mime.o list.o hashmap.o
RM = rm -f

# Build rule
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

srvd: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean install uninstall

# Cleanup
clean:
	$(RM) $(OBJ) srvd

# Installation configuration
PREFIX ?= /usr/local
DESTDIR ?=
BIN_DIR = $(PREFIX)/bin
CONF_DIR = /etc/srv
WEB_DIR = /var/www/srv

# Installation targets
install: srvd
	@echo "Installing srvd to $(DESTDIR)$(BIN_DIR)..."
	@mkdir -p $(DESTDIR)$(BIN_DIR)
	@install -m 755 srvd $(DESTDIR)$(BIN_DIR)
	@echo "Installing configuration to $(DESTDIR)$(CONF_DIR)..."
	@mkdir -p $(DESTDIR)$(CONF_DIR)
	@install -m 644 server.conf $(DESTDIR)$(CONF_DIR)
	@install -m 644 mime.types $(DESTDIR)$(CONF_DIR)
	@echo "Installing web content to $(DESTDIR)$(WEB_DIR)..."
	@mkdir -p $(DESTDIR)$(WEB_DIR)
	@install -m 644 index.html $(DESTDIR)$(WEB_DIR)

uninstall:
	@echo "Removing srvd from $(DESTDIR)$(BIN_DIR)..."
	@$(RM) $(DESTDIR)$(BIN_DIR)/srvd
	@echo "Removing configuration from $(DESTDIR)$(CONF_DIR)..."
	@$(RM) -r $(DESTDIR)$(CONF_DIR)
	@echo "Removing web content from $(DESTDIR)$(WEB_DIR)..."
	@$(RM) -r $(DESTDIR)$(WEB_DIR)

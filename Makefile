CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -Iinclude
LDLIBS  := $(shell pkg-config --libs ncurses 2>/dev/null || echo -lncurses) \
           $(shell pkg-config --libs libcurl 2>/dev/null || echo -lcurl)

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := crescent-tui

.PHONY: all clean install

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(BIN)
	install -Dm755 $(BIN) $(DESTDIR)/usr/local/bin/$(BIN)

clean:
	rm -f $(OBJ) $(BIN)
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -Iinclude

LDLIBS  := $(shell pkg-config --libs ncurses 2>/dev/null || echo -lncurses) \
           $(shell pkg-config --libs libcurl 2>/dev/null || echo -lcurl)

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := crescent-tui

WIN_CC      := x86_64-w64-mingw32-gcc
WIN_CFLAGS  := -std=c11 -Wall -Wextra -O2 -Iinclude
WIN_LDLIBS  := -lcurl -lpdcurses -lws2_32
WIN_BIN     := crescent-tui.exe

.PHONY: all clean install windows clean-windows

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

windows:
	$(WIN_CC) $(WIN_CFLAGS) $(SRC) -o $(WIN_BIN) $(WIN_LDLIBS)

install: $(BIN)
	install -Dm755 $(BIN) $(DESTDIR)/usr/local/bin/$(BIN)

clean:
	rm -f $(OBJ) $(BIN)

clean-windows:
	rm -f $(WIN_BIN)
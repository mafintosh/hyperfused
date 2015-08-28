PREFIX ?= /usr/local
SRC_C = src/enc.c src/id_map.c src/socket.c
SRC_H = src/enc.h src/id_map.h src/socket.h
FUSE_OPTS = $(shell pkg-config --libs-only-L --libs-only-l fuse --cflags-only-I)-D_FILE_OFFSET_BITS=64
CC = gcc -std=c99

all: hyperfuse

hyperfuse: $(SRC_C) $(SRC_H) src/hyperfuse.c
	$(CC) -O3 -pthread $(SRC_C) src/hyperfuse.c $(FUSE_OPTS) -o $@

install: hyperfuse
	cp hyperfuse $(PREFIX)/bin/hyperfuse

uninstall:
	rm -f $(PREFIX)/bin/hyperfuse

test: $(SRC_C) $(SRC_H) tests/*.c
	$(CC) $(SRC_C) tests/*.c $(FUSE_OPTS) -o $@
	./$@

clean:
	rm -f test hyperfuse

.PHONY: test clean install uninstall

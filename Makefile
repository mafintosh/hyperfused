PREFIX ?= /usr/local
SRC_C = src/enc.c src/id_map.c src/socket.c
SRC_H = src/enc.h src/id_map.h src/socket.h
FUSE_OPTS = $(shell pkg-config --libs-only-L --libs-only-l fuse --cflags-only-I)-D_FILE_OFFSET_BITS=64
CC = gcc -std=c99

all: hyperfused

hyperfused: $(SRC_C) $(SRC_H) src/hyperfused.c
	$(CC) -O3 $(SRC_C) src/hyperfused.c $(FUSE_OPTS) -lpthread -Ipthread -o $@

install: hyperfused
	cp hyperfused $(PREFIX)/bin/hyperfused

uninstall:
	rm -f $(PREFIX)/bin/hyperfused

test: $(SRC_C) $(SRC_H) tests/*.c
	$(CC) $(SRC_C) tests/*.c $(FUSE_OPTS) -o $@
	./$@

clean:
	rm -f test hyperfused

.PHONY: test clean install uninstall

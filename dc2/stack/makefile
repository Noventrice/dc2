CFLAGS=-ggdb -Wall -Wextra -Wpedantic -O3
CC=clang
bins=demo test

.PHONY: all
all: $(bins)

demo: demo.c

.PHONY: clean
clean:
	rm -f $(bins)

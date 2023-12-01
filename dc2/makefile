#O ?= 0 #This would work if O wasn't already an export variable (sh$ O='';).
ifndef O
	O=0
endif

ifeq ($(origin CC), default)
	CC=clang
endif
#Reasons for C23:
	#f() instead of f(void)
	#Using declarations before (case) labels.
		#Currently not implemented in the compiler.
	#Lets you use bool without stdbool.h or _Bool.
		#Although, I use stdbool.h due to starting with requiring C11.
#Reasons for C11:
	#Anonymous structs.
	#Compound literals.
CC+=-std=c2x

CFLAGS=-ggdb3 -Wall -Wextra -Wpedantic -pipe -O$(O)
LDLIBS=
bins=tokenizer dc2 hash
test_bins=test/f test/tok_table
objs=tokenizer.o hash.o value.o
man_dir=/usr/local/share/man/man1
bin_dir=/usr/local/bin

stack_dir=./stack
#These are dependencies but the compilation command doesn't need them.
imports=funct_cases.inc errf.h isatty.h

.PHONY: all
all: $(bins) $(objs)

.PHONY: sanitize
sanitize: CFLAGS+=-fsanitize=address
sanitize: all test

release: O=3
release: CFLAGS+=-fstack-protector
release: all

%: %.c
	$(CC) $(CFLAGS) $(LDLIBS) $(filter-out $(imports),$^) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

tokenizer: CFLAGS+=-DTOKENIZER_MAIN -iquote $(stack_dir)
tokenizer: LDLIBS+=-lreadline
tokenizer: $(imports) tokenizer.c value.o

#tokenizer.o: tokenizer.c #Implicit.

dc2: CFLAGS+=-iquote $(stack_dir)
dc2: LDLIBS+=-lm -lreadline
dc2: $(objs) $(imports) dc2.c

hash: CFLAGS+=-DHASH_MAIN
hash: value.o hash.c

.PHONY: lines
lines:
	@wc -l $(wildcard *.c) $(wildcard *.h) $(wildcard test/*.c) \
	./test/test makefile funct_cases.inc | sort -k 2;

.PHONY: root_check install uninstall
root_check:
	@if [ "$$UID" != '0' ];then printf 'dc2: Need root!\n' >&2; exit 1; fi

install: root_check release #Check for root before compiling.
	mkdir -p '$(man_dir)' && cp dc2.1.gz '$(man_dir)';
	mkdir -p '$(bin_dir)' && cp dc2 '$(bin_dir)';

uninstall: root_check
	rm -f '$(man_dir)/dc2.1.gz' '$(bin_dir)/dc2';

test/f: CFLAGS+=-iquote $(stack_dir)
test/f: test/f.c

test/tok_table: test/tok_table.c value.o

.PHONY: test
test: $(test_bins)
	./test/test

.PHONY: clean
clean:
	rm -f $(bins) $(objs) fifo a.out in.gdb log $(test_bins)


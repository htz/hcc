CC := gcc
CFLAGS=-Wall -Wno-strict-aliasing -std=gnu11 -g -I. -O0 -DBUILD_DIR='"$(shell pwd)"'

PROG := hcc
SRCS := builtin.c cpp.c error.c file.c gen.c lex.c macro.c main.c map.c node.c parse.c string.c token.c type.c util.c vector.c
OBJS := ${SRCS:%.c=%.o}
DEPS := ${SRCS:%.c=%.d}
TESTS := $(patsubst %.c,%.out,$(filter-out test/testmain.c, $(wildcard test/*.c)))

.PHONY: all
all: ${PROG}

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${CFLAGS}

%.o: %.c
	${CC} $< -MM -MP -MF $*.d ${CFLAGS}
	${CC} -c $< -o $@ ${CFLAGS}

.PHONY: test
test: ${PROG} ${TESTS}
	./test.sh
	@for test in $(TESTS); do \
		./$$test;               \
	done

test/%.s: test/%.c ${PROG}
	./${PROG} < $< > $@

test/%.out: test/%.s test/testmain.c ${PROG}
	@$(CC) $(CFLAGS) -no-pie -o $@ $< test/testmain.c

.PHONY: clean
clean:
	${RM} ${PROG} ${OBJS} ${DEPS} ${TESTS}

ifeq ($(findstring clean,${MAKECMDGOALS}),)
  -include ${DEPS}
endif

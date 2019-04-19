CC := gcc
CFLAGS=-Wall -Wno-strict-aliasing -std=gnu11 -g -I. -O0

PROG := hcc
SRCS := error.c gen.c lex.c main.c map.c node.c parse.c string.c token.c util.c vector.c
OBJS := ${SRCS:%.c=%.o}
DEPS := ${SRCS:%.c=%.d}

.PHONY: all
all: ${PROG}

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${CFLAGS}

%.o: %.c
	${CC} $< -MM -MP -MF $*.d ${CFLAGS}
	${CC} -c $< -o $@ ${CFLAGS}

.PHONY: test
test: ${PROG}
	./test.sh

.PHONY: clean
clean:
	${RM} ${PROG} ${OBJS} ${DEPS}

ifeq ($(findstring clean,${MAKECMDGOALS}),)
  -include ${DEPS}
endif

# Makefile

PROG = nec86play
SRCS = nec86hw.c nec86play.c
LDFLAGS += -lm
NOMAN = 1

.include <bsd.prog.mk>

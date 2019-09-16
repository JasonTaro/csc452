# This is the Makefile for Phase 0. You should not modify anything.
# It provides the following targets:
#
#	make		(makes phase0)
#
#	make phase0 (makes phase0)
#	make clean	(removes all files created by this Makefile)

ifndef PREFIX
	PREFIX = $(HOME)
endif

USLOSS_VERSION = 3.6

SRCS = phase0.c

CFLAGS = -Wall -g

TARGET = phase0

INCLUDES = -I$(PREFIX)/include -I.

ifeq ($(shell uname),Darwin)
	DEFINES += -D_XOPEN_SOURCE
	OS = macosx
	CFLAGS += -Wno-int-to-void-pointer-cast -Wno-extra-tokens -Wno-unused-label -Wno-unused-function
else
	OS = linux
	CFLAGS += -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-unused-but-set-variable
endif

CC=gcc
LD=gcc
AR=ar    
CFLAGS += $(INCLUDES) $(DEFINES)
LDFLAGS = -L. -L$(PREFIX)/lib
COBJS = ${SRCS:.c=.o}
DEPS = ${COBJS:.o=.d}
LIBS = -lusloss$(USLOSS_VERSION)


# The following is to deal with circular dependencies between the USLOSS, phase 1, and phase 2
# libraries. Unfortunately the linkers handle this differently on the two OSes.

ifeq ($(OS), macosx)
	LIBFLAGS = -Wl,-all_load $(LIBS)
else
	LIBFLAGS = -Wl,--start-group $(LIBS) -Wl,--end-group
endif

%.d: %.c
	$(CC) -c $(CFLAGS) -MM -MF $@ $<

all: $(TARGET)

$(TARGET):  $(COBJS)
	$(LD) $(LDFLAGS) -o $@ $@.o $(LIBFLAGS)	$(LIBS)

clean:
	rm -f $(COBJS) $(TARGET) $(DEPS)

tar: $(TARGET)-starter.tgz

$(TARGET)-starter.tgz: $(SRCS) Makefile
	tar cvzf $@ $^

-include $(DEPS)

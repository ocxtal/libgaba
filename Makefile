CC = gcc
AR = ar
GIT = git
ARCHFLAGS = -march=native
CFLAGS = -O3 -Wall -Wno-unused-function -Wno-unused-label -std=c99 -pipe
TARGET = libgaba.o

all: native

native:
	$(MAKE) -f Makefile.core CC=$(CC) CFLAGS='$(CFLAGS) $(ARCHFLAGS)'
	$(AR) rcs $(TARGET) gaba.*.o

unittest:
	$(MAKE) -f Makefile.core CC=$(CC) CFLAGS='$(CFLAGS) $(ARCHFLAGS)'
	$(CC) -o unittest unittest.c gaba.*.o

clean:
	rm -rf *.o $(TARGET) *~ *.a *.dSYM session*

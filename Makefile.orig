CC=gcc
CFLAGS=-Wall -Werror -g
SRCDIR=src
TESTDIR=tests
TEST_FLAGS=-I$(SRCDIR) -lcheck
LIBS=-lev -lsockutils -lbeanstalkclient -lioqueue
OBJECTS=evbsc.o

all: libevbsc.so

check: $(TESTDIR)/evbsc.t
	$(TESTDIR)/evbsc.t  

$(TESTDIR)/evbsc.t: $(TESTDIR)/check_evbsc.c evbsc.o
	$(CC) $(TEST_FLAGS) -o $(TESTDIR)/evbsc.t $(TESTDIR)/check_evbsc.c $(OBJECTS) $(LIBS)

libevbsc.so: $(OBJECTS)
	$(CC) $(CFLAGS) --shared -o libevsc.so $(OBJECTS) $(LIBS)

evbsc.o:    $(SRCDIR)/evbsc.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/evbsc.c

clean:
	rm -f *.so *.o tests/*.t

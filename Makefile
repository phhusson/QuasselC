CFLAGS:=-Wall -g -Wextra $(shell pkg-config glib-2.0 --cflags) -Wswitch-enum -std=gnu11 -O2 -fPIC
SO_VERSION = 0
VERSION = 0
INSTALL = install
LDLIBS:=$(shell pkg-config glib-2.0 --libs) -lz

lib_objects=setters.o getters.o main.o cmds.o display.o negotiation.o io.o

all: bot libquasselc.so.$(VERSION) quasselc.pc

libquasselc.so.$(VERSION): $(lib_objects)
	$(CC) -shared -o $@ -Wl,-soname,libquasselc.so.$(SO_VERSION) $^ $(LDLIBS)

bot: bot.o libquasselc.so.$(VERSION)
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o bot libquasselc.so.$(VERSION) quasselc.pc

quasselc.pc: quasselc.pc.in
	sed -e 's/@version@/$(VERSION)/;' < $^ > $@

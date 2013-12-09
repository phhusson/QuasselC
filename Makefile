CFLAGS:=-Wall -g -Wextra $(shell pkg-config glib-2.0 --cflags) -Wswitch-enum -std=gnu11 -O2
LDLIBS:=$(shell pkg-config glib-2.0 --libs)

objects=bot.o setters.o getters.o main.o cmds.o display.o

all: bot

bot: $(objects)

clean:
	rm -f $(objects)

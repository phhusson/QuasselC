CFLAGS:=-Wall -g

objects=bot.o setters.o endian.o getters.o main.o cmds.o display.o

all: bot

bot: $(objects)

clean:
	rm -f $(objects)

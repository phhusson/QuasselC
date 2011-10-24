#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>
#include "quasselc.h"

void handle_message(int fd, struct message m) {
	//printf("%s:%d:%d:%s\n", m.sender, m.type, m.flags, m.content);
	char *nick=strdup(m.sender);
	if(index(nick, '!'))
		*index(nick, '!')=0;
	if(m.type==1) {
		printf("%s %s: %s\n", nick, m.buffer.name, m.content);
	}
	if(m.type==2) {
		printf("%s %s %s\n", nick, m.buffer.name, m.content);
	}
	free(nick);
	if(strstr(m.sender, "phh2")==m.sender) {
		send_message(fd, m.buffer, "/SAY hello");
	}
}

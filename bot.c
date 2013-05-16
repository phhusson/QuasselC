/*
   This file is part of QuasselC.

   QuasselC is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   QuasselC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with QuasselC.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <asm/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>
#include "quasselc.h"

#if 0
#define dprintf(x...) printf(x)
#else
#define dprintf(x...)
#endif

static int fd=-1;
static int highlight=0;

struct buffer {
	struct bufferinfo i;
	int lastseen;
	int marker;
	int displayed;
};
static struct buffer *buffers;
static int n_buffers;
static int requester;
static char *match;

static int find_buffer_id(char *name) {
	int i;
	for(i=0;i<n_buffers;++i) {
		if(buffers[i].i.id==-1)
			continue;
		if(strcmp(buffers[i].i.name, name)==0)
			return i;
	}
	return -1;
}

void handle_backlog(int fd, struct message m) {
	if(!match)
		return;
	if(!strstr(m.content, match))
		return;
	char msg[512];
	char *nick=strdup(m.sender);
	if(index(nick, '!'))
		*index(nick, '!')=0;
	snprintf(msg, 511, "%s: %s", nick, m.content);
	free(nick);
	msg[511]=0;
	send_message(fd, buffers[requester].i, msg);
}

void handle_message(int fd, struct message m) {
	char *nick=strdup(m.sender);
	if(index(nick, '!'))
		*index(nick, '!')=0;
	if(m.type==1) {
		printf("%s %s: %s\n", nick, m.buffer.name, m.content);
	}
	if(m.type==2) {
		printf("%s %s; %s\n", nick, m.buffer.name, m.content);
	}
	free(nick);
	if(strstr(m.content, "phh"))
		highlight=1;
	if(!index("#&!+", m.buffer.name[0]))
		highlight=1;

	if(index("#&!+", m.buffer.name[0]))
		return;

	if(strstr(m.content, "!lastlog ")==m.content) {
		requester=m.buffer.id;
		match=m.content+strlen("!lastlog ");
		requestBacklog(fd, find_buffer_id("#quassel"), -1, -1, 60, 0);
	}

	if(strstr(m.sender, "phh2")==m.sender) {
		send_message(fd, m.buffer, "/SAY hello");
	}
}

void handle_sync(object_t o, function_t f, ...) {
	va_list ap;
	char *fnc=NULL;
	char *net,*chan,*nick,*name;
	int netid,bufid,msgid;
	va_start(ap, f);
	switch(f) {
		/* BufferSyncer */
		case Create:
			bufid=va_arg(ap, int);
			netid=va_arg(ap, int);
			name=va_arg(ap, char*);
			dprintf("CreateBuffer(%d, %d, %s)\n", netid, bufid, name);
			if(bufid>=n_buffers) {
				buffers=realloc(buffers, sizeof(struct buffer)*(bufid+1));
				int i;
				for(i=n_buffers;i<=bufid;++i)
					buffers[i].i.id=-1;
				n_buffers=bufid+1;
			}
			buffers[bufid].i.network=netid;
			buffers[bufid].i.id=bufid;
			buffers[bufid].i.name=name;
			buffers[bufid].marker=0;
			buffers[bufid].lastseen=0;
			buffers[bufid].displayed=1;
			break;
		case MarkBufferAsRead:
			highlight=0;
			if(!fnc)
				fnc="MarkBufferAsRead";
		case Displayed:
			if(!fnc)
				fnc="BufferDisplayed";
			bufid=va_arg(ap, int);
			dprintf("%s(%d)\n", fnc, bufid);
			buffers[bufid].displayed=1;
			break;
		case Removed:
			if(!fnc)
				fnc="BufferRemoved";
		case TempRemoved:
			if(!fnc)
				fnc="BufferTempRemoved";
			bufid=va_arg(ap, int);
			buffers[bufid].displayed=0;
			dprintf("%s(%d)\n", fnc, bufid);
			break;
		case SetLastSeenMsg:
			if(!fnc)
				fnc="SetLastSeenMsg";
			bufid=va_arg(ap, int);
			msgid=va_arg(ap, int);
			buffers[bufid].lastseen=msgid;
			dprintf("%s(%d, %d)\n", fnc, bufid, msgid);
			break;
		case SetMarkerLine:
			if(!fnc)
				fnc="SetMarkerLine";
			bufid=va_arg(ap, int);
			msgid=va_arg(ap, int);
			buffers[bufid].marker=msgid;
			dprintf("%s(%d, %d)\n", fnc, bufid, msgid);
			break;
		/* IrcChannel */
		case JoinIrcUsers:
			net=va_arg(ap, char*);
			chan=va_arg(ap, char*);
			int size=va_arg(ap, int);
			char **users=va_arg(ap, char**);
			char **modes=va_arg(ap, char**);
			if(size==0)
				break;
			if(size>1) {
				printf("Too many users joined\n");
				break;
			}
			dprintf("JoinIrcUser(%s, %s, %s, %s)\n", net, chan, users[0], modes[0]);
			break;
		case AddUserMode:
			if(!fnc)
				fnc="AddUserMode";
		case RemoveUserMode:
			if(!fnc)
				fnc="RemoveUserMode";
			net=va_arg(ap, char*);
			chan=va_arg(ap, char*);
			nick=va_arg(ap, char*);
			char *mode=va_arg(ap, char*);
			dprintf("%s(%s, %s, %s, %s)\n", fnc, net, chan, nick, mode);
			break;
		/* IrcUser */
		case SetNick2:
			if(!fnc)
				fnc="SetNick";
		case Quit:
			if(!fnc)
				fnc="Quit";
			net=va_arg(ap, char*);
			nick=va_arg(ap, char*);
			dprintf("%s(%s, %s)\n", fnc, net, nick);
			break;
		case SetNick:
			if(!fnc)
				fnc="SetNick";
		case SetServer:
			if(!fnc)
				fnc="SetServer";
		case SetRealName:
			if(!fnc)
				fnc="SetRealName";
		case PartChannel:
			if(!fnc)
				fnc="PartChannel";
			net=va_arg(ap, char*);
			nick=va_arg(ap, char*);
			char *str=va_arg(ap, char*);
			dprintf("%s(%s, %s, %s)\n", fnc, net, nick, str);
			break;
		case SetAway:
			net=va_arg(ap, char*);
			nick=va_arg(ap, char*);
			int away=va_arg(ap, int);
			dprintf("setAway(%s, %s, %s)\n", net, nick, away ? "away" : "present");
			break;
		/* Network */
		case AddIrcUser:
			net=va_arg(ap, char*);
			char *name=va_arg(ap, char*);
			dprintf("AddIrcUser(%s, %s)\n", net, name);
			break;
		case SetLatency:
			net=va_arg(ap, char*);
			int latency=va_arg(ap, int);
			dprintf("SetLatency(%s, %d)\n", net, latency);
			break;
	}
	va_end(ap);
}

static int accept_fd;
void handle_fd(int fd) {
	if(fd==accept_fd) {
		int client_fd=accept(fd, NULL, NULL);
		if(client_fd>=0) {
			char str[]="0\n";
			str[0]+=highlight;
			write(client_fd, str, 2);
			close(client_fd);
		}
	}
}

static void __init() __attribute__((constructor));
static void __init() {
	struct sockaddr_in6 addr;
	bzero(&addr, sizeof(addr));
	fd = socket(AF_INET6, SOCK_STREAM, 0);
	addr.sin6_family=AF_INET6;
	addr.sin6_port=htons(4242);
	bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(fd, 42);
	accept_fd=fd;
	register_fd(fd);

	int reuse = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

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
#include "export.h"

void handle_backlog(struct message m, void *arg) {
	(void) arg;
	(void) m;
}

void handle_message(struct message m, void *arg) {
	(void) arg;
	(void) m;
}

void handle_sync(void* arg, object_t o, function_t f, ...) {
	(void) arg;
	(void) f;
	(void) o;
}

static char* user = NULL;
static char* pass = NULL;
void handle_event(void* arg, GIOChannel* h, event_t t, ...) {
	(void) arg;
	va_list ap;
	va_start(ap, t);
	switch(t) {
		case ClientInitAck:
			if (user != NULL && pass != NULL)
				quassel_login(h, user, pass);
			else {
				printf("OK - quasselcore init acked (not logged in)\n");
				exit(0);
			}
			break;
		case SessionInit:
			printf("OK - quasselcore login successful\n");
			exit(0);
			break;
		case TopicChange:
			break;
		case ChanPreAddUser:
			break;
		case ChanReady:
			break;
	}
	va_end(ap);
}

typedef struct {
	char *msg;
	uint32_t size;
	uint32_t got;
} net_buf;

static int io_handler(GIOChannel *chan, GIOCondition condition, gpointer data) {
	(void) condition;
	net_buf *b = (net_buf*)data;
	if(!b->size) {
		uint32_t size;
		if(read_io(chan, (char*)&size, 4)!=4)
			return 1;
		size=htonl(size);
		if(size==0)
			return 1;
		b->msg = malloc(size);
		if(!b->msg)
			return 1;
		b->size = size;
		b->got = 0;
	}

	b->got += read_io(chan, b->msg+b->got, b->size-b->got);
	if(b->got == b->size) {
		quassel_parse_message(chan, b->msg, NULL);
		//display_qvariant(b->msg);
		free(b->msg);
		b->got = 0;
		b->size = 0;
	}
	return 1;
}

static int create_socket(const char* host, const char* port) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(host, port, &hints, &result);
	if (s != 0) {
		printf("CRITICAL - getaddrinfo: %s\n", gai_strerror(s));
		exit(2);
	}

	/* getaddrinfo() returns a list of address structures.
	   Try each address until we successfully bind(2).
	   If socket(2) (or bind(2)) fails, we (close the socket
	   and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;                  /* Success */

		close(sfd);
	}

	if (rp == NULL) {               /* No address succeeded */
		printf("CRITICAL - Could not bind\n");
		exit(2);
	}

	freeaddrinfo(result);
	return sfd;
}

static gboolean critical_exit(gpointer data) {
	(void) data;
	printf("CRITICAL - Timed out setting up session\n");
	exit(2);
}

int main(int argc, char **argv) {
	if(argc < 3) {
		fprintf(stderr, "%s: <host> <port> [<user> <pass>]\n", argv[0]);
		return 1;
	}

	g_timeout_add_seconds(10, critical_exit, NULL); 

	net_buf b;
	b.msg = NULL;
	b.size = 0;
	b.got = 0;

	user = argv[3];
	pass = argv[4];
	GIOChannel *in = g_io_channel_unix_new(create_socket(argv[1], argv[2]));
	g_io_channel_set_encoding(in, NULL, NULL);
	g_io_channel_set_buffered(in, FALSE);

	if (!quassel_negotiate(in, 0)) {
		in = g_io_channel_unix_new(create_socket(argv[1], argv[2]));
		g_io_channel_set_encoding(in, NULL, NULL);
		g_io_channel_set_buffered(in, FALSE);
	}

	quassel_init_packet(in, 0);
	// FIXME: Add timeout
	g_io_add_watch(in, G_IO_IN, io_handler, &b);
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	exit(0);
}

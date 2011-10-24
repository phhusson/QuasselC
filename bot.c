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

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

void Login(int fd, char *user, char *pass) {
	//HeartBeat
	char msg[2048];
	int size;
	int elements;

	size=0;
	elements=0;
	bzero(msg, sizeof(msg));

	size+=add_string_in_map(msg+size, "MsgType", "ClientLogin");
	elements++;

	size+=add_string_in_map(msg+size, "User", user);
	elements++;

	size+=add_string_in_map(msg+size, "Password", pass);
	elements++;

	//The message will be of that length
	uint32_t v=ltob(size+8);
	write(fd, &v, 4);
	//This is a QMap
	v=ltob(8);
	write(fd, &v, 4);
	
	//QMap is valid
	v=0;
	write(fd, &v, 1);
	//The QMap has <elements> elements
	v=ltob(elements);
	write(fd, &v, 4);
	write(fd, msg, size);
}

void HeartbeatReply(int fd) {
	//HeartBeat
	char msg[2048];
	int size;

	size=0;
	bzero(msg, sizeof(msg));

	//QVariant<QList<QVariant>> of 2 elements
	size+=add_qvariant(msg+size, 9);
	size+=add_int(msg+size, 2);

	//QVariant<Int> = HeartbeatReply
	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, 6);

	//QTime
	size+=add_qvariant(msg+size, 15);
	//I don't have any clue what time it is...
	size+=add_int(msg+size, 0);

	//The message will be of that length
	uint32_t v=ltob(size+4);
	write(fd, &v, 4);
	write(fd, msg, size);
}

void send_message(int fd, struct bufferinfo b, char *message) {
	printf("Sending message ! \n");
	//HeartBeat
	char msg[2048];
	int size;

	size=0;
	bzero(msg, sizeof(msg));

	//QVariant<QList<QVariant>> of 4 elements
	size+=add_qvariant(msg+size, 9);
	size+=add_int(msg+size, 4);

	//QVariant<Int> = RPC
	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, 2);

	//QVariant<Bytearray> = "2sendInput(BufferInfo,QString)"
	size+=add_qvariant(msg+size, 12);
	size+=add_bytearray(msg+size, "2sendInput(BufferInfo,QString)");

	//QVariant<BufferInfo>
	size+=add_qvariant(msg+size, 127);
	size+=add_bytearray(msg+size, "BufferInfo");
	size+=add_bufferinfo(msg+size, b);

	//String
	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, message);

	//The message will be of that length
	uint32_t v=ltob(size);
	write(fd, &v, 4);
	write(fd, msg, size);
}

void initRequest(int fd, char *val, char *arg) {
	char msg[2048];
	int size;
	
	size=0;
	bzero(msg, sizeof(msg));

	//QVariant<QList<QVariant>> of 3 elements
	size+=add_qvariant(msg+size, 9);
	size+=add_int(msg+size, 3);

	//QVariant<Int> = InitRequest
	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, 3);

	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, val);

	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, arg);

	//The message will be of that length
	uint32_t v=ltob(size+4);
	write(fd, &v, 4);
	write(fd, msg, size);
}

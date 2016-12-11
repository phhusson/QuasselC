/*
   This file is part of QuasselC.

   QuasselC is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3.0 of the License, or (at your option) any later version.

   QuasselC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with QuasselC.  If not, see <http://www.gnu.org/licenses/>.
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
#include "export.h"

int quassel_negotiate(GIOChannel* h, int ssl) {
	uint32_t magic = 0x42b33f00;

	magic |= !!ssl; //SSL
	magic |= 2; //Compression
	
	magic = htonl(magic);
	write_io(h, (char*)&magic, sizeof(magic));

	//Send supported protocols
	{
		//Legacy protocol
		uint32_t v = 1;
		v = htonl(v);
		write_io(h, (char*)&v, sizeof(v));

		//End of list
		v = 1 << 31;
		v = htonl(v);
		write_io(h, (char*)&v, sizeof(v));
	}

	//Read answer
	uint32_t response;
	while(!read_io(h, (char*)&response, sizeof(response)));
	response = ntohl(response);

	//No support for legacy protocol
	if( (response&0xff)!= 1)
		return 0;

	if( (response>>24) & 1) {
		//switch to ssl
	}

	if( (response>>24) & 2) {
		//switch and compression
		extern int useCompression;
		useCompression = 1;
	}
	return 1;
}

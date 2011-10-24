#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>
#include "quasselc.h"

int connect_to(char *hostname, char *port) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo(hostname, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for(rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if(sfd == -1)
			continue;

		if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;                  /* Success */

		close(sfd);
	}

	if(!rp) {               /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		return -1;
	}

	freeaddrinfo(result);           /* No longer needed */
	return sfd;
}

void parse_message(char *buf, int fd) {
	int type=get_qvariant(&buf);
	if(type==9) {
		//List, "normal" mode
		int size=get_int(&buf);
		type=get_qvariant(&buf);
		if(type!=2 && size) {
			printf("Expected int there ... %d\n", __LINE__);
			return;
		}
		int cmd=get_int(&buf);
		switch(cmd) {
			case 1:
				//Sync
				break;
			case 2:
				//RPC
				type=get_qvariant(&buf);
				if(type!=12) {
					printf("Expected Bytearray, got %d(line=%d)\n", type, __LINE__);
					if(type==10)
						printf("Expected Bytearray, got string %s (line=%d)\n", get_string(&buf), __LINE__);
					return;
				}
				char *cmd_str=get_bytearray(&buf);
				char *type_str;
				if(strcmp(cmd_str, "2displayMsg(Message)")==0) {
					type=get_qvariant(&buf);
					if(type!=127) {
						printf("Expected UserType, got %d(line=%d)\n", type, __LINE__);
						return;
					}
					type_str=get_bytearray(&buf);
					if(strcmp(type_str, "Message")) {
						printf("Expected UserType::Message, got %s(line=%d)\n", type_str, __LINE__);
						return;
					}
					struct message m = get_message(&buf);
					handle_message(fd, m);
				}
				free(cmd_str);
				break;
			case 3:
				//InitRequest
				break;
			case 4:
				//InitData
				break;
			case 5:
				HeartbeatReply(fd);
				break;
			case 6:
				//HeartbeatReply;
				break;
		}
	} else if(type==8) {
		//QMap, probably somewhere in the init
		int size=get_int(&buf);
		int i;
		for(i=0;i<size;++i) {
			char *key=get_string(&buf);
			type=get_qvariant(&buf);
			if(strcmp(key, "MsgType")==0) {
				if(type!=10) {
					printf("Unexpected type: %d (line %d)\n", type, __LINE__);
					abort();
				}
				char *category=get_string(&buf);
				if(strcmp(category, "ClientInitAck")==0)
					Login(fd, "phh", "azerty");
				continue;
			} else if(strcmp(key, "SessionState")==0) {
				int size2=get_int(&buf);
				int j;
				for(j=0;j<size2;++j) {
					char *key2=get_string(&buf);
					int type2=get_qvariant(&buf);
					if(strcmp(key2, "BufferInfos")==0) {
						if(type2!=9) {
							printf("Unexpected type: %d (%d)\n", type2, __LINE__);
							abort();
						}
						int size3=get_int(&buf);
						int k;
						for(k=0;k<size3;++k) {
							get_qvariant(&buf);
							get_bytearray(&buf);
							struct bufferinfo m=get_bufferinfo(&buf);
							//TODO save list of bufferinfos somewhere to be able to send messages
						}
						continue;
					}
					get_variant_t(&buf, type2);
				}
				continue;
			}
			get_variant_t(&buf, type);
		}
	}
}

void read_and_display(int fd) {
	uint32_t size;
	read(fd, &size, 4);
	size=ltob(size);
	if(size==0)
		exit(-1);
	char *buf=malloc(size);
	bzero(buf, size);
	int done=0;
	while(done<size) {
		done+=read(fd, buf+done, size-done);
	}

	//Enable this to debug
	//display_qvariant(buf);
	parse_message(buf, fd);
	free(buf);
}

int main(int argc, char **argv) {
	int fd = connect_to("localhost", "4242");
	if(fd==-1)
		return -1;

	int size=0;
	int elements=0;
	char msg[2048];

	size+=add_bool_in_map(msg+size, "UseSsl", 0);
	elements++;

	size+=add_bool_in_map(msg+size, "UseCompression", 0);
	elements++;

	size+=add_int_in_map(msg+size, "ProtocolVersion", 10);
	elements++;

	size+=add_string_in_map(msg+size, "MsgType", "ClientInit");
	elements++;

	size+=add_string_in_map(msg+size, "ClientVersion", "v0.6.1 (dist-<a href='http://git.quassel-irc.org/?p=quassel.git;a=commit;h=611ebccdb6a2a4a89cf1f565bee7e72bcad13ffb'>611ebcc</a>)");
	elements++;

	size+=add_string_in_map(msg+size, "ClientDate", "Oct 23 2011 18:00:00");
	elements++;

	//The message will be of that length
	unsigned int v=ltob(size+5);
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

	while(1)
		read_and_display(fd);
	return 0;
}

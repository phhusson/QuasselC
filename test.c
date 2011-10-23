#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>

int connect_to(char *hostname, char *port) {
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
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

#define add_msg(value) { \
	memcpy(msg+size, value, sizeof(value));\
	size+=sizeof(value); \
	elements++; \
}


unsigned long long int lltob(unsigned long long int a) {
	unsigned long long int ret;
	int i;
	for(i=0;i<8;++i) {
		ret = a&0xff;
		ret <<= 8;
		a>>=8;
		ret |= a&0xff;
	}
	return ret;
}

uint32_t ltob(uint32_t a) {
	unsigned long int ret;
	int i;
	for(i=0;i<4;++i) {
		ret = a&0xff;
		ret <<= 8;
		a>>=8;
		ret |= a&0xff;
	}
	return ret;
}

unsigned short stob(unsigned short a) {
	unsigned short ret;
	int i;
	for(i=0;i<4;++i) {
		ret = a&0xff;
		ret <<= 8;
		a>>=8;
		ret |= a&0xff;
	}
	return ret;
}

static iconv_t ico;
char *convert_string(char *str, int* size) {
	int s1,s2;
	char buf[512];
	char *pos=buf;
	s1=strlen(str);
	s2=sizeof(buf);
	iconv(ico, &str, &s1, &pos, &s2);
	if(str[0]!=0) {
		fprintf(stderr, "iconv failed ! \n");
	}
	*size=sizeof(buf)-s2;
	return buf;
}

int add_string_in_map(char *msg, char *key, char *value) {
	int pos=0;

	//Key
	msg[pos]=ltob(10);
	pos+=4;
	int size;
	char *tmp=convert_string(key, &size);
	msg[pos]=ltob(size);
	pos+=4;
	memcpy(msg+pos, tmp, size);
	pos+=size;

	//Value
	msg[pos]=ltob(10);
	pos+=4;
	tmp=convert_string(value, &size);
	msg[pos]=ltob(size);
	pos+=4;
	memcpy(msg+pos, tmp, size);
	pos+=size;

	return pos;
}

int add_bool_in_map(char *msg, char *key, int value) {
	int pos=0;

	//Key
	msg[pos]=ltob(10);
	pos+=4;
	int size;
	char *tmp=convert_string(key, &size);
	msg[pos]=ltob(size);
	pos+=4;
	memcpy(msg+pos, tmp, size);
	pos+=size;

	//Value
	msg[pos]=ltob(1);
	pos+=4;
	msg[pos]=value;
	pos++;

	return pos;
}

int add_int_in_map(char *msg, char *key, uint32_t value) {
	int pos=0;

	//Key
	msg[pos]=ltob(10);
	pos+=4;
	int size;
	char *tmp=convert_string(key, &size);
	msg[pos]=ltob(size);
	pos+=4;
	memcpy(msg+pos, tmp, size);
	pos+=size;

	//Value
	msg[pos]=ltob(2);
	pos+=4;
	msg[pos]=value;
	pos+=4;

	return pos;
}

int main(int argc, char **argv) {
	ico = iconv_open("UTF-16BE//TRANSLIT", "UTF-8");
	int fd = connect_to("localhost", "4242");
	if(fd==-1)
		return -1;

	int size=0;
	int elements=0;
	char msg[1024];

	size+=add_string_in_map(msg+size, "ClientDate", "Oct 23 2011 18:00:00");
	elements++;

	size+=add_bool_in_map(msg+size, "UseSsl", 0);
	elements++;

	size+=add_string_in_map(msg+size, "ClientVersion", "v0.6.1 (dist-<a href='http://git.quassel-irc.org/?p=quassel.git;a=commit;h=611ebccdb6a2a4a89cf1f565bee7e72bcad13ffb'>611ebcc</a>)");
	elements++;

	size+=add_bool_in_map(msg+size, "UseCompression", 0);
	elements++;

	size+=add_string_in_map(msg+size, "MsgType", "ClientInit");
	elements++;

	size+=add_int_in_map(msg+size, "ProtocolVersion", 10);
	elements++;

	//The message will be of that length
	write(fd, ltob(size+8), 4);
	//This is a QMap
	write(fd, ltob(4), 4);
	//The QMap has <elements> elements
	write(fd, ltob(elements), 4);
	write(fd, msg, size);

	bzero2(msg, size);
	size = read(fd, msg, sizeof(msg));
	int i;
	for(i=0;i<size;++i)
		printf("%c", size[i]);
	printf("\n");
}

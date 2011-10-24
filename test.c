#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>

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

unsigned long long int lltob(unsigned long long int a) {
	unsigned long long int ret;
	int i;
	for(i=0;i<8;++i) {
		ret <<= 8;
		ret |= a&0xff;
		a>>=8;
	}
	return ret;
}

uint32_t ltob(uint32_t a) {
	uint32_t ret=0;
	int i;
	for(i=0;i<4;++i) {
		ret <<= 8;
		ret |= a&0xff;
		a>>=8;
	}
	return ret;
}

unsigned short stob(unsigned short a) {
	unsigned short ret;
	int i;
	for(i=0;i<4;++i) {
		ret <<= 8;
		ret |= a&0xff;
		a>>=8;
	}
	return ret;
}

static iconv_t ico,ico2;
char *convert_string(char *str, int* size) {
	size_t s1,s2;
	static char buf[512];
	char *pos=buf;
	bzero(buf, sizeof(buf));
	s1=strlen(str);
	s2=sizeof(buf);
	iconv(ico, &str, &s1, &pos, &s2);
	if(str[0]!=0) {
		fprintf(stderr, "iconv failed ! \n");
		abort();
	}
	*size=sizeof(buf)-s2;
	return buf;
}

char *convert_back(char *str, int orig_size, int* size) {
	size_t s1,s2;
	static char buf[512];
	char *pos=buf;
	bzero(buf, sizeof(buf));
	s1=orig_size;
	s2=sizeof(buf);
	iconv(ico2, &str, &s1, &pos, &s2);
	if(str[0]!=0) {
		fprintf(stderr, "iconv failed ! \n");
		abort();
	}
	*size=sizeof(buf)-s2;
	return buf;
}

int add_string(char *msg, char *str) {
	int size=0;
	char *tmp=convert_string(str, &size);
	*(uint32_t*)(msg)=ltob(size);
	memcpy(msg+4, tmp, size);
	return size+4;
}

int add_int(char *msg, uint32_t v) {
	*(uint32_t*)(msg)=ltob(v);
	return 4;
}

int add_qvariant(char *msg, int type) {
	*(uint32_t*)(msg)=ltob(type);
	msg[4]=0;
	return 5;
}

int add_string_in_map(char *msg, char *key, char* value) {
	char *buf=msg;

	//Key
	buf+=add_string(msg, key);

	//Value
	buf+=add_qvariant(buf, 10);
	buf+=add_string(buf, value);

	return buf-msg;
}

int add_bool_in_map(char *msg, char *key, int value) {
	char *buf=msg;

	//Key
	buf+=add_string(msg, key);

	//Value
	buf+=add_qvariant(buf, 1);
	*buf=value;
	buf++;

	return buf-msg;
}

int add_int_in_map(char *msg, char *key, int value) {
	char *buf=msg;

	//Key
	buf+=add_string(msg, key);

	//Value
	buf+=add_qvariant(buf, 3);
	buf+=add_int(buf, value);

	return buf-msg;
}

int display_qvariant(char *buf);
int display_map(char *buf);
int display_int(char *buf);
int display_short(char *buf);
int display_bytearray(char *buf);

int display_string(char *buf) {
	uint32_t size = *((uint32_t*)buf);
	size = ltob(size);
	if(size==0xffffffff) {
		printf(" Str()");
		return 4;
	}
	buf+=4;
	int res=0;
	char *str=convert_back(buf, size, &res);
	printf(" Str('%s')", str);
	return size+4;
}

int display_bufferinfo(char *buf) {
	uint32_t id = *((uint32_t*)buf);
	id=ltob(id);
	buf+=4;

	uint32_t networkID=*((uint32_t*)buf);
	networkID=ltob(networkID);
	buf+=4;

	uint16_t type=*((uint16_t*)buf);
	type=stob(type);
	buf+=2;

	uint32_t groupID=*((uint32_t*)buf);
	groupID=ltob(groupID);
	buf+=4;

	uint32_t size = *((uint32_t*)buf);
	size = ltob(size);
	buf+=4;

	if(size==0xffffffff) {
		printf("Size was <0 !\n");
		size=0;
	}
	printf("BufferInfo(%d, %d, %d, %d, '%s')", id, networkID, type, groupID, buf);
	return 18+size;
}

int display_message(char *buf) {
	char *orig_buf=buf;
	uint32_t messageID=*((uint32_t*)buf);
	messageID=ltob(messageID);
	buf+=4;

	uint32_t timestamp=*((uint32_t*)buf);
	timestamp=ltob(timestamp);
	buf+=4;

	uint32_t type=*((uint32_t*)buf);
	type=ltob(type);
	buf+=4;

	char flags=*buf;
	buf++;

	printf("Message(%d, %d, %d, %d, ", messageID, timestamp, type, flags);
	buf+=display_bufferinfo(buf);

	printf(", ");
	//Sender
	buf+=display_bytearray(buf);
	printf(", ");
	//Content
	buf+=display_bytearray(buf);

	printf(")\n");

	return buf-orig_buf;
}

int display_bytearray(char *buf) {
	uint32_t size = *((uint32_t*)buf);
	size = ltob(size);
	if(size==0xffffffff) {
		printf("Bytearray()");
		return 4;
	} else {
		char *str=malloc(size+1);
		memcpy(str, buf+4, size);
		str[size]=0;
		printf("Bytearray('%s')", str);
		free(str);
	}
	return 4+size;
}

int display_usertype(char *buf) {
	char *orig_buf=buf;
	uint32_t size = *((uint32_t*)buf);
	size = ltob(size);
	buf+=4;
	if(strcmp(buf, "NetworkId")==0 || strcmp(buf, "IdentityId")==0 || strcmp(buf, "BufferId")==0 || strcmp(buf, "MsgId")==0) {
		buf+=strlen(buf)+1;
		buf+=display_int(buf);
	} else if(strcmp(buf, "Identity")==0) {
		buf+=strlen(buf)+1;
		buf+=display_map(buf);
	} else if(strcmp(buf, "BufferInfo")==0) {
		buf+=strlen(buf)+1;
		buf+=display_bufferinfo(buf);
	} else if(strcmp(buf, "Message")==0) {
		buf+=strlen(buf)+1;
		buf+=display_message(buf);
	} else if(strcmp(buf, "Network::Server")==0) {
		buf+=strlen(buf)+1;
		buf+=display_map(buf);
	} else {
		printf(" Usertype('%s') \n", buf);
		printf("Unsupported.\n");
		exit(0);
	}
	return buf-orig_buf;
}

int display_int(char *buf) {
	uint32_t size = *((uint32_t*)buf);
	size = ltob(size);
	printf(" Int(%08x) \n", size);
	return 4;
}

int display_short(char *buf) {
	uint16_t size = *((uint16_t*)buf);
	size = stob(size);
	printf(" Short(%04x) \n", size);
	return 2;
}

int display_date(char *buf) {
	uint32_t julianDay = *((uint32_t*)buf);
	julianDay=ltob(julianDay);
	buf+=4;
	uint32_t secondSinceMidnight = *((uint32_t*)buf);
	secondSinceMidnight=ltob(secondSinceMidnight);
	buf+=4;
	uint32_t isUTC = *buf;
	buf++;
	printf(" Date(%d, %d, %d) \n", julianDay, secondSinceMidnight, isUTC);
	return 9;
}

int display_time(char *buf) {
	uint32_t secondsSinceMidnight = *((uint32_t*)buf);
	secondsSinceMidnight=ltob(secondsSinceMidnight);
	printf(" Time(%d) \n", secondsSinceMidnight);
	return 4;
}

int display_bool(char *buf) {
	printf(" Bool(%s) \n", *buf ? "TRUE" : "FALSE");
	return 1;
}

int display_map(char *buf) {
	char *orig_buf=buf;
	uint32_t elements = *((uint32_t*)buf);
	elements = ltob(elements);
	printf("Got %d elements\n", elements);
	printf("Map(\n");
	buf+=4;
	int i;
	for(i=0;i<elements;++i) {
		//key
		printf("key= ");
		buf+=display_string(buf);
		printf(", ");
		//Value
		printf("value= ");
		buf+=display_qvariant(buf);
		printf("\n");
	}
	printf(");\n");
	return buf-orig_buf;
}

int display_list(char *buf) {
	char *orig_buf=buf;
	uint32_t elements = *((uint32_t*)buf);
	elements = ltob(elements);
	printf("Got %d elements\n", elements);
	printf("List(\n");
	buf+=4;
	int i;
	for(i=0;i<elements;++i) {
		//Value
		printf("value= ");
		buf+=display_qvariant(buf);
		printf("\n");
	}
	printf(");\n");
	return buf-orig_buf;
}

int display_stringlist(char *buf) {
	char *orig_buf=buf;
	uint32_t elements = *((uint32_t*)buf);
	elements = ltob(elements);
	printf("Got %d elements\n", elements);
	printf("StringList(\n");
	buf+=4;
	int i;
	for(i=0;i<elements;++i) {
		//Value
		printf("value= ");
		buf+=display_string(buf);
	}
	printf(");\n");
	return buf-orig_buf;
}

int display_qvariant(char *buf) {
	char *orig_buf=buf;
	uint32_t type = *((uint32_t*)buf);
	type=ltob(type);
	buf+=4;
	char null=*buf;
	buf++;
	if(null) {
		//Nothing to do
	}
	switch(type) {
		case 1:
			buf+=display_bool(buf);
			break;
		case 2:
		case 3:
			buf+=display_int(buf);
			break;
		case 8:
			buf+=display_map(buf);
			break;
		case 9:
			buf+=display_list(buf);
			break;
		case 10:
			buf+=display_string(buf);
			break;
		case 11:
			buf+=display_stringlist(buf);
			break;
		case 12:
			buf+=display_bytearray(buf);
			break;
		case 15:
			buf+=display_time(buf);
			break;
		case 16:
			buf+=display_date(buf);
			break;
		case 127:
			//User type !
			buf+=display_usertype(buf);
			break;
		case 133:
			buf+=display_short(buf);
			break;
		default:
			printf("Unknown QVariant type: %d\n", type);
			exit(-1);
			break;
	};
	return buf-orig_buf;
}

uint32_t get_int(char **buf);
uint16_t get_short(char **buf);
char get_byte(char **buf);
char* get_bytearray(char **buf);

struct bufferinfo {
	uint32_t id;
	uint32_t network;
	uint16_t type;
	uint32_t group;
	char *name;
};

struct bufferinfo get_bufferinfo(char **buf) {
	struct bufferinfo b;
	b.id=get_int(buf);
	b.network=get_int(buf);
	b.type=get_short(buf);
	b.group=get_int(buf);
	b.name=get_bytearray(buf);
	return b;
}

struct message {
	uint32_t id;
	uint32_t timestamp;
	uint32_t type;
	char flags;
	struct bufferinfo buffer;
	char *sender;
	char *content;
};

struct message get_message(char **buf) {
	struct message m;
	m.id=get_int(buf);
	m.timestamp=get_int(buf);
	m.type=get_int(buf);
	m.flags=get_byte(buf);
	m.buffer=get_bufferinfo(buf);
	m.sender=get_bytearray(buf);
	m.content=get_bytearray(buf);

	return m;
}

void get_date(char **buf) {
	(*buf)+=9;
}

char* get_bytearray(char **buf) {
	uint32_t size = *((uint32_t*)*buf);
	size=ltob(size);
	(*buf)+=4;
	if(size==0xFFFFFFFF)
		return strdup("");
	char *ret=strdup(*buf);
	(*buf)+=size;
	return ret;
}

char* get_string(char **buf) {
	uint32_t size = *((uint32_t*)*buf);
	size=ltob(size);
	(*buf)+=4;

	if(size==0xffffffff) {
		return strdup("");
	}
	int res=0;
	char *str=convert_back(*buf, size, &res);
	(*buf)+=size;
	return strdup(str);
}

char get_byte(char **buf) {
	char v = **buf;
	(*buf)++;
	return v;
}

uint32_t get_int(char **buf) {
	uint32_t v = *((uint32_t*)*buf);
	v=ltob(v);
	(*buf)+=4;
	return v;
}

uint16_t get_short(char **buf) {
	uint16_t v = *((uint16_t*)*buf);
	v=stob(v);
	(*buf)+=2;
	return v;
}

int get_qvariant(char **buf) {
	uint32_t type = *((uint32_t*)*buf);
	type=ltob(type);
	(*buf)+=4;
	char null=**buf;
	if(null) {
		//NOP
	}
	(*buf)++;
	return type;
}

void get_list(char **buf);
void get_stringlist(char **buf);
void get_map(char **buf);

void get_variant_t(char **buf, int type) {
	switch(type) {
		case 1:
			get_byte(buf);
			break;
		case 2:
		case 3:
			get_int(buf);
			break;
		case 8:
			get_map(buf);
			break;
		case 9:
			get_list(buf);
			break;
		case 10:
			get_string(buf);
			break;
		case 11:
			get_stringlist(buf);
			break;
		case 16:
			get_date(buf);
			break;
		case 127:
			{
				char *usertype=get_bytearray(buf);
				if(strcmp(usertype, "NetworkId")==0 || strcmp(usertype, "IdentityId")==0) {
					get_int(buf);
				} else if(strcmp(usertype, "Identity")==0) {
					get_map(buf);
				} else if(strcmp(usertype, "BufferInfo")==0) {
					get_bufferinfo(buf);
				} else {
					printf("Unsupported usertype = %s (%d)\n", usertype, __LINE__);
					abort();
				}
			}
			break;
		default:
			abort();
	}
}

void get_variant(char **buf) {
	int type=get_qvariant(buf);
	get_variant_t(buf, type);
}

void get_map(char **buf) {
	int size=get_int(buf);
	int i;
	for(i=0;i<size;++i) {
		get_string(buf);
		get_variant(buf);
	}
}

void get_stringlist(char **buf) {
	int size=get_int(buf);
	int i;
	for(i=0;i<size;++i) {
		get_string(buf);
	}
}

void get_list(char **buf) {
	int size=get_int(buf);
	int i;
	for(i=0;i<size;++i) {
		get_variant(buf);
	}
}

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

	size+=add_string_in_map(msg+size, "User", "phh");
	elements++;

	size+=add_string_in_map(msg+size, "Password", "azerty");
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
	int elements;

	size=0;
	elements=0;
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
					if(m.type&3)
						printf("A message from %s, on %s: %s! \n", m.sender, m.buffer.name, m.content);
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
							printf("Buffer name = %s\n", m.name);
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

	//display_qvariant(buf);
	parse_message(buf, fd);
}

void initRequest(int fd, char *val, char *arg) {
	char msg[2048];
	int size;
	int elements;
	
	size=0;
	elements=0;
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

int main(int argc, char **argv) {
	ico = iconv_open("UTF-16BE//TRANSLIT", "UTF-8");
	ico2 = iconv_open("UTF-8", "UTF-16BE");
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
	unsigned int v=ltob(size+8);
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

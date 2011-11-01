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

static char* login;
static char* pass;

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

int parse_message(char *buf, int fd) {
	int type=get_qvariant(&buf);
	if(type==9) {
		//List, "normal" mode
		int size=get_int(&buf);
		type=get_qvariant(&buf);
		if(type!=2 && size) {
			printf("Expected int there ... %d\n", __LINE__);
			return 1;
		}
		int cmd=get_int(&buf);
		switch(cmd) {
			case 1:
				{
					//Sync(=RPC on objects)

					//Object type
					type=get_qvariant(&buf);
					char *cmd_str;
					if(type==10)
						cmd_str=get_string(&buf);
					else if(type==12)
						cmd_str=get_bytearray(&buf);
					else
						return 1;

					//Constructor parameter (always string?)
					type=get_qvariant(&buf);
					if(type!=10)
						return 1;
					char *arg=get_string(&buf);
					type=get_qvariant(&buf);


					//Function from this class called
					if(type!=12)
						return 1;
					char *fnc;
					fnc=get_bytearray(&buf);

					if(!strcmp(cmd_str, "BufferSyncer")) {
						free(cmd_str);
						free(arg);
						
						if(!strcmp(fnc, "markBufferAsRead")) {
							//BufferSyncer::markBufferAsRead(int bufferid)
							free(fnc);

							type=get_qvariant(&buf);
							if(type!=127)
								return 1;
							char *type_str=get_bytearray(&buf);
							if(strcmp(type_str, "BufferId"))
								return 1;
							free(type_str);

							int id = get_int(&buf);
							handle_sync(BufferSyncer, MarkBufferAsRead, id);
							return 0;
						} else if(!strcmp(fnc, "setLastSeenMsg")) {
							//BufferSyncer::setLastSeenMsg(int, int)
							free(fnc);

							type=get_qvariant(&buf);
							if(type!=127)
								return 1;
							char *type_str=get_bytearray(&buf);
							if(strcmp(type_str, "BufferId"))
								return 1;
							free(type_str);
							int bufferid = get_int(&buf);

							type=get_qvariant(&buf);
							if(type!=127)
								return 1;
							type_str=get_bytearray(&buf);
							if(strcmp(type_str, "MsgId"))
								return 1;
							free(type_str);
							int messageid = get_int(&buf);
							handle_sync(BufferSyncer, SetLastSeenMsg, bufferid, messageid);
							return 0;
						} else if(!strcmp(fnc, "setMarkerLine")) {
							//BufferSyncer::setMarkerLine(int, int)
							free(fnc);

							type=get_qvariant(&buf);
							if(type!=127)
								return 1;
							char *type_str=get_bytearray(&buf);
							if(strcmp(type_str, "BufferId"))
								return 1;
							free(type_str);
							int bufferid = get_int(&buf);

							type=get_qvariant(&buf);
							if(type!=127)
								return 1;
							type_str=get_bytearray(&buf);
							if(strcmp(type_str, "MsgId"))
								return 1;

							free(type_str);
							int messageid = get_int(&buf);
							handle_sync(BufferSyncer, SetLastSeenMsg, bufferid, messageid);
							return 0;
						}
					} else if(!strcmp(cmd_str, "IrcChannel")) {
						free(cmd_str);
						char *network=arg;
						char *chan;
						if(!index(network, '/'))
							return 1;
						arg=index(network, '/');
						arg[0]=0;
						arg++;
						chan=arg;

						if(!strcmp(fnc, "joinIrcUsers")) {
							free(fnc);
							//Arguments are two QStringList, nicks then modes
							//nick list
							type=get_qvariant(&buf);
							if(type!=11)
								return 1;
							int size=get_int(&buf);
							int i;
							char **users=malloc(size*sizeof(char*));
							for(i=0;i<size;++i) {
								char *nick=get_string(&buf);
								users[i]=nick;
							}

							//mode list
							type=get_qvariant(&buf);
							if(type!=11)
								return 1;
							if(size!=get_int(&buf))
								return 1;
							char **modes=malloc(size*sizeof(char*));
							for(i=0;i<size;++i) {
								char *mode=get_string(&buf);
								modes[i]=mode;
							}
							handle_sync(IrcChannel, JoinIrcUsers, network, chan, size, users, modes);
							free(network);
							for(i=0;i<size;++i) {
								free(users[i]);
								free(modes[i]);
							}
							free(users);
							free(modes);
							return 0;
						} else if(!strcmp(fnc, "addUserMode")) {
							//IrcChannel::addUserMode(QString nick, QString mode)
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *nick=get_string(&buf);

							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *mode=get_string(&buf);

							handle_sync(IrcChannel, AddUserMode, network, chan, nick, mode);
							free(nick);
							free(mode);
							free(network);
							return 0;
						} else if(!strcmp(fnc, "removeUserMode")) {
							//IrcChannel::removeUserMode(QString nick, QString mode)
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *nick=get_string(&buf);

							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *mode=get_string(&buf);

							handle_sync(IrcChannel, RemoveUserMode, network, chan, nick, mode);
							free(nick);
							free(mode);
							free(network);
							return 0;
						}

						free(fnc);
						free(network);
						return 1;
					} else if(!strcmp(cmd_str, "IrcUser")) {
						free(cmd_str);
						char *network=arg;
						char *nick;
						if(!index(network, '/'))
							return 1;
						arg=index(network, '/');
						arg[0]=0;
						arg++;
						nick=arg;

						if(!strcmp(fnc, "quit")) {
							//IrcUser::quit();
							handle_sync(IrcUser, Quit, network, nick);
							free(fnc);
							free(network);
							return 0;
						} else if(!strcmp(fnc, "setServer")) {
							//IrcUser::setServer(QString);
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *srv=get_string(&buf);
							handle_sync(IrcUser, SetServer, network, nick, srv);
							free(srv);
							free(network);
							return 0;
						} else if(!strcmp(fnc, "setRealName")) {
							//IrcUser::setRealName(QString);
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *name=get_string(&buf);
							handle_sync(IrcUser, SetRealName, network, nick, name);
							free(name);
							free(network);
							return 0;
						} else if(!strcmp(fnc, "setAway")) {
							//IrcUser::setAway(bool);
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=1)
								return 1;
							int away=get_byte(&buf);
							handle_sync(IrcUser, SetAway, network, nick, away);
							free(network);
							return 0;
						} else if(!strcmp(fnc, "setNick")) {
							//IrcUser::setNick(QString)
							//We receive this after we got __objectRenamed__
							//So that's kind of useless
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *nick=get_string(&buf);
							handle_sync(IrcUser, SetNick2, network, nick);
							free(nick);
							free(network);
							return 0;
						} else if(!strcmp(fnc, "partChannel")) {
							//IrcUser::partChannel(QString)
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *chan=get_string(&buf);
							handle_sync(IrcUser, PartChannel, network, nick, chan);
							free(chan);
							free(network);
							return 0;
						}
					} else if(!strcmp(cmd_str, "Network")) {
						free(cmd_str);

						if(!strcmp(fnc, "addIrcUser")) {
							//Network::addIrcUser(QString)
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *fullname=get_string(&buf);
							handle_sync(Network, AddIrcUser, arg, fullname);
							free(fullname);
							free(arg);
							return 0;
						} else if(!strcmp(fnc, "setLatency")) {
							//Network::addIrcUser(int)
							free(fnc);
							type=get_qvariant(&buf);
							if(type!=2)
								return 1;
							int latency=get_int(&buf);
							handle_sync(Network, SetLatency, arg, latency);
							free(arg);
							return 0;
						}
					} else if(!strcmp(cmd_str, "BacklogManager")) {
						free(cmd_str);

						if(!strcmp(fnc, "receiveBacklog") || !strcmp(fnc, "receiveBacklogAll")) {
							//BacklogManager::receiveBacklog
							free(arg);
							int bufid=-1;
							char *type_str;

							if(!strcmp(fnc, "receiveBacklog")) {
								type=get_qvariant(&buf);
								if(type!=127)
									return 1;
								type_str=get_bytearray(&buf);
								if(strcmp(type_str, "BufferId"))
									return 1;
								bufid=get_int(&buf);
							}
							free(fnc);

							type=get_qvariant(&buf);
							if(type!=127)
								return 1;
							type_str=get_bytearray(&buf);
							if(strcmp(type_str, "MsgId"))
								return 1;
							int first=get_int(&buf);

							type=get_qvariant(&buf);
							if(type!=127)
								return 1;
							type_str=get_bytearray(&buf);
							if(strcmp(type_str, "MsgId"))
								return 1;
							int last=get_int(&buf);

							type=get_qvariant(&buf);
							if(type!=2)
								return 1;
							int limit=get_int(&buf);

							type=get_qvariant(&buf);
							if(type!=2)
								return 1;
							int additional=get_int(&buf);

							type=get_qvariant(&buf);
							if(type!=9)
								return 1;
							int size=get_int(&buf);
							int i;
							for(i=0;i<size;++i) {
								type=get_qvariant(&buf);
								if(type!=127)
									return 1;
								type_str=get_bytearray(&buf);
								if(strcmp(type_str, "Message"))
									return 1;
								struct message m = get_message(&buf);
								handle_backlog(fd, m);
							}
							return 0;
						}
					} else
						printf("Got unknown sync object type:%s\n", cmd_str);
				}
				break;
			case 2:
				{
					//RPC
					type=get_qvariant(&buf);
					char *cmd_str;
					if(type==12)
						cmd_str=get_bytearray(&buf);
					else if(type==10)
						cmd_str=get_string(&buf);
					else {
						printf("Expected Bytearray, got %d(line=%d)\n", type, __LINE__);
						return 1;
					}
					char *type_str;
					if(strcmp(cmd_str, "2displayMsg(Message)")==0) {
						type=get_qvariant(&buf);
						if(type!=127) {
							printf("Expected UserType, got %d(line=%d)\n", type, __LINE__);
							return 1;
						}
						type_str=get_bytearray(&buf);
						if(strcmp(type_str, "Message")) {
							printf("Expected UserType::Message, got %s(line=%d)\n", type_str, __LINE__);
							return 1;
						}
						struct message m = get_message(&buf);
						handle_message(fd, m);
						free(cmd_str);
						return 0;
					} else if(!strcmp(cmd_str, "__objectRenamed__")) {
						type=get_qvariant(&buf);
						if(type!=12)
							return 1;
						char *type_str=get_bytearray(&buf);
						if(strcmp(type_str, "IrcUser")) {
							printf("Unknown object renaming ! %s\n", type_str);
							return 1;
						} else {

							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *orig=get_string(&buf);
							type=get_qvariant(&buf);
							if(type!=10)
								return 1;
							char *net=orig;
							if(!index(orig, '/'))
								return 1;
							orig=index(orig, '/');
							orig[0]=0;
							orig++;
							char *new=get_string(&buf);
							char *new_nick=new;
							if(!index(new, '/'))
								return 1;
							new_nick=index(new, '/')+1;
							handle_sync(IrcUser, SetNick, net, orig, new_nick);
							free(new);
							free(net);
							return 0;
						}
					} else
						printf("Got cmd = '%s'\n", cmd_str);
					free(cmd_str);
					return 1;
				}
				break;
			case 3:
				//InitRequest
				return 1;
				break;
			case 4:
				//InitData
				{
					type=get_qvariant(&buf);
					char *cmd_str;
					if(type!=12)
						return 1;
					cmd_str=get_bytearray(&buf);
					if(!strcmp(cmd_str, "BufferViewConfig")) {
						type=get_qvariant(&buf);
						if(type!=10)
							return 1;
						char *arg=get_string(&buf);
						if(strcmp(arg, "0"))
							return 1;
						free(arg);

						//Now comes a QMap, need to browse it
						type=get_qvariant(&buf);
						if(type!=8)
							return 1;
						int size=get_int(&buf);
						int i;
						for(i=0;i<size;++i) {
							char *key=get_string(&buf);
							if(!strcmp(key, "TemporarilyRemovedBuffers")) {
								type=get_qvariant(&buf);
								if(type!=9)
									return 1;
								int n=get_int(&buf);
								int j;
								for(j=0;j<n;++j) {
									type=get_qvariant(&buf);
									if(type!=127)
										return 1;
									char *type_str=get_bytearray(&buf);
									if(strcmp(type_str, "BufferId"))
										return 1;
									int bufid=get_int(&buf);
									handle_sync(BufferSyncer, TempRemoved, bufid);
								}
							} else if(!strcmp(key, "RemovedBuffers")) {
								type=get_qvariant(&buf);
								if(type!=9)
									return 1;
								int n=get_int(&buf);
								int j;
								for(j=0;j<n;++j) {
									type=get_qvariant(&buf);
									if(type!=127)
										return 1;
									char *type_str=get_bytearray(&buf);
									if(strcmp(type_str, "BufferId"))
										return 1;
									int bufid=get_int(&buf);
									handle_sync(BufferSyncer, Removed, bufid);
								}
							} else if(!strcmp(key, "BufferList")) {
								type=get_qvariant(&buf);
								if(type!=9)
									return 1;
								int n=get_int(&buf);
								int j;
								for(j=0;j<n;++j) {
									type=get_qvariant(&buf);
									if(type!=127)
										return 1;
									char *type_str=get_bytearray(&buf);
									if(strcmp(type_str, "BufferId"))
										return 1;
									int bufid=get_int(&buf);
									handle_sync(BufferSyncer, Displayed, bufid);
								}
							} else {
								get_variant(&buf);
							}
						}
						return 0;
					} else if(!strcmp(cmd_str, "BufferSyncer")) {
						type=get_qvariant(&buf);
						if(type!=10)
							return 1;
						char *arg=get_string(&buf);
						if(strcmp(arg, ""))
							return 1;
						free(arg);

						//Now comes a QMap, need to browse it
						type=get_qvariant(&buf);
						if(type!=8)
							return 1;
						int size=get_int(&buf);
						int i;
						for(i=0;i<size;++i) {
							char *key=get_string(&buf);
							if(!strcmp(key, "MarkerLines")) {
								type=get_qvariant(&buf);
								if(type!=9)
									return 1;
								int n=get_int(&buf);
								if(n%2)
									return 1;
								int j;
								for(j=0;j<n/2;++j) {
									type=get_qvariant(&buf);
									if(type!=127)
										return 1;
									char *type_str=get_bytearray(&buf);
									if(strcmp(type_str, "BufferId"))
										return 1;
									int bufid=get_int(&buf);

									type=get_qvariant(&buf);
									if(type!=127)
										return 1;
									type_str=get_bytearray(&buf);
									if(strcmp(type_str, "MsgId"))
										return 1;
									int msgid=get_int(&buf);

									handle_sync(BufferSyncer, SetMarkerLine, bufid, msgid);
								}
							} else if(!strcmp(key, "LastSeenMsg")) {
								type=get_qvariant(&buf);
								if(type!=9)
									return 1;
								int n=get_int(&buf);
								if(n%2)
									return 1;
								int j;
								for(j=0;j<n/2;++j) {
									type=get_qvariant(&buf);
									if(type!=127)
										return 1;
									char *type_str=get_bytearray(&buf);
									if(strcmp(type_str, "BufferId"))
										return 1;
									int bufid=get_int(&buf);

									type=get_qvariant(&buf);
									if(type!=127)
										return 1;
									type_str=get_bytearray(&buf);
									if(strcmp(type_str, "MsgId"))
										return 1;
									int msgid=get_int(&buf);

									handle_sync(BufferSyncer, SetLastSeenMsg, bufid, msgid);
								}
							} else {
								get_variant(&buf);
							}
						}
						return 0;
					}
					return 1;
				}
				break;
			case 5:
				HeartbeatReply(fd);
				return 0;
				break;
			case 6:
				//HeartbeatReply;
				return 1;
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
					return 1;
				}
				char *category=get_string(&buf);
				if(strcmp(category, "ClientInitAck")==0)
					Login(fd, login, pass);
				else if(strcmp(category, "SessionInit")==0) {
					//Get buffers' display status
					initRequest(fd, "BufferViewConfig", "0");
					//Retreive marker lines and last seen msg
					initRequest(fd, "BufferSyncer", "");
					//initRequest(fd, "Network", "5");
					//requestBacklog(fd, 29, -1, -1, 5, 0);
				}
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
							return 1;
						}
						int size3=get_int(&buf);
						int k;
						for(k=0;k<size3;++k) {
							get_qvariant(&buf);
							get_bytearray(&buf);
							struct bufferinfo m=get_bufferinfo(&buf);
							handle_sync(BufferSyncer, Create, m.id, m.network, m.name);
						}
						continue;
					}
					get_variant_t(&buf, type2);
				}
				continue;
			}
			get_variant_t(&buf, type);
		}
		return 0;
	}
	return 1;
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
	if(parse_message(buf, fd))
		display_qvariant(buf);
	free(buf);
}

typedef struct fd_list {
	int fd;
	struct fd_list *n;
} fd_list;
static fd_list *a=NULL;
void register_fd(int fd) {
	fd_list *b;
	if(!a) {
		a=malloc(sizeof(fd_list));
		b=a;
	} else {
		b=a;
		while(b->n)
			b=b->n;
		b->n=malloc(sizeof(fd_list));
		b=b->n;
		b->n=NULL;
	}
	b->fd=fd;
}

int main(int argc, char **argv) {
	if(argc!=5)
		printf("Usage: %s login password host port\n", argv[0]);
	login=argv[1];
	pass=argv[2];
	int fd = connect_to(argv[3], argv[4]);
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

	while(1) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		fd_list *x=a;
		while(x) {
			FD_SET(x->fd, &fds);
			x=x->n;
		}
		select(42, &fds, NULL, NULL, NULL);
		if(FD_ISSET(fd, &fds))
			read_and_display(fd);
		x=a;
		while(x) {
			if(FD_ISSET(x->fd, &fds))
				handle_fd(x->fd);
			x=x->n;
		}
	}
	return 0;
}

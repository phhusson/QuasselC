#ifndef CMDS_H
#define CMDS_H
#include "types.h"

void Login(int fd, char *user, char *pass);
void HeartbeatReply(int fd);
void send_message(int fd, struct bufferinfo b, char *message);
void initRequest(int fd, char *val, char *arg);
void requestBacklog(int fd, int buffer, int first, int last, int limit, int additional);
#endif

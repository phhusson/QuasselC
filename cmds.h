#ifndef CMDS_H
#define CMDS_H
#include "types.h"

extern void quassel_login(GIOChannel *h, char *user, char *pass);
extern void HeartbeatReply(GIOChannel*);
extern void send_message(GIOChannel*, struct bufferinfo b, const char *message);
extern void initRequest(GIOChannel*, char *val, char *arg);
extern void quassel_request_backlog(GIOChannel *h, int buffer, int first, int last, int limit, int additional);
extern void quassel_perm_hide(GIOChannel *h, int buffer);
extern void quassel_temp_hide(GIOChannel *h, int buffer);
#endif

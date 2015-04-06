#ifndef QUASSEL_EXPORT_H
#define QUASSEL_EXPORT_H

/* quassel lib */
extern void quassel_send_message(GIOChannel* h, int buffer, const char *message);
extern void quassel_mark_as_read(GIOChannel* h, int buffer_id);
extern void quassel_set_last_seen_msg(GIOChannel* h, int buffer_id, int msg_id);
extern void quassel_set_marker(GIOChannel* h, int buffer_id, int msgid);
extern void quassel_login(GIOChannel* h, char *user, char *pass);
extern void quassel_init_packet(GIOChannel* h, int ssl);
extern int quassel_parse_message(GIOChannel* h, char *buf, void *arg);
extern void quassel_request_backlog(GIOChannel *h, int buffer, int first, int last, int limit, int additional);
extern int quassel_find_buffer_id(const char *name, uint32_t network);
extern int quassel_negotiate(GIOChannel* h, int ssl);

extern int read_io(GIOChannel* h, char *buf, int len);
extern int write_io(GIOChannel* h, const char *buf, int len);
extern void quassel_perm_hide(GIOChannel *h, int buffer);
extern void quassel_temp_hide(GIOChannel *h, int buffer);
#endif

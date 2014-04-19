#ifndef IO_H
#define IO_H

extern int write_io(GIOChannel *handle, const char* data, int len);
extern int read_io(GIOChannel *handle, char *buf, int len);

#endif /* IO_H */

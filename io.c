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

//Copy paste from irssi's write_io
int write_io(GIOChannel *handle, const char* data, int len) {
	gsize ret;
	GIOStatus status;
	GError *err = NULL;

	g_return_val_if_fail(handle != NULL, -1);
	g_return_val_if_fail(data != NULL, -1);

	status = g_io_channel_write_chars(handle, (char *) data, len, &ret, &err);
	if (err != NULL) {
		g_warning("%s", err->message);
		g_error_free(err);
	}
	if (status == G_IO_STATUS_ERROR)
		return -1;

	return ret;

}

//Copy/pasted from irssi/network.c
int read_io(GIOChannel *handle, char *buf, int len)
{
	gsize ret;
	GIOStatus status;
	GError *err = NULL;

	g_return_val_if_fail(handle != NULL, -1);
	g_return_val_if_fail(buf != NULL, -1);

	status = g_io_channel_read_chars(handle, buf, len, &ret, &err);
	if (err != NULL) {
		g_warning(err->message);
		g_error_free(err);
	}
	if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
		return -1; /* disconnected */

	return ret;
}

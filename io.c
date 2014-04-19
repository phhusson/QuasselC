#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>
#include <zlib.h>
#include "quasselc.h"
#include "export.h"

int useCompression = 0;
//Copy paste from irssi's write_io
int write_io(GIOChannel *handle, const char* data, int len) {
	gsize ret;
	GIOStatus status;
	GError *err = NULL;

	g_return_val_if_fail(handle != NULL, -1);
	g_return_val_if_fail(data != NULL, -1);

	if(useCompression) {
		static void* ZBuf;
		static int ZBuf_size;

		static z_stream ZOutStream;
		static int enabled;
		if(!enabled) {
			if(deflateInit(&ZOutStream, 9)) {
				fprintf(stderr, "deflateInit failed\n");
				exit(1);
			}
			enabled = 1;
		}

		if(!ZBuf) {
			ZBuf = malloc(1024);
			ZBuf_size = 1024;
		}

		while(1) {
			ZOutStream.avail_out = ZBuf_size;
			ZOutStream.next_out = ZBuf;

			ZOutStream.avail_in = len;
			ZOutStream.next_in = (char*)data;

			int ret = deflate(&ZOutStream, Z_SYNC_FLUSH);
			if(ret) {
				fprintf(stderr, "deflate failed: %d\n", ret);
				exit(1);
			}
			if(ZOutStream.avail_out != 0) {
				len = ZBuf_size - ZOutStream.avail_out;
				break;
			}
			ZBuf = realloc(ZBuf, ZBuf_size+1024);
			ZBuf_size+=1024;
		}

		data = ZBuf;
	}

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
	static z_stream ZInStream;
	gsize ret;
	GIOStatus status;
	GError *err = NULL;

	g_return_val_if_fail(handle != NULL, -1);
	g_return_val_if_fail(buf != NULL, -1);

	char *Rbuf = buf;
	int Rlen = len;
	if(useCompression) {
		static int enabled;
		if(!enabled) {
			if(inflateInit(&ZInStream)) {
				fprintf(stderr, "deflateInit failed\n");
				exit(1);
			}
			enabled = 1;
		}

		static char ZBuf[1024];
		Rbuf = ZBuf;
		Rlen = sizeof(ZBuf);

		//Check if there is some buffer left...
		//If there is, return it
		if(ZInStream.avail_in) {
			int origRet = len;
			ZInStream.avail_out = len;
			ZInStream.next_out = buf;
			int zret = inflate(&ZInStream, Z_SYNC_FLUSH);
			if(zret) {
				fprintf(stderr, "inflate failed: %d\n", zret);
				exit(1);
			}
			ret = len - ZInStream.avail_out;
			return ret;
		}
	}

	status = g_io_channel_read_chars(handle, Rbuf, Rlen, &ret, &err);
	if (err != NULL) {
		g_warning("%s\n", err->message);
		g_error_free(err);
	}
	if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
		return -1; /* disconnected */

	if(useCompression) {

		int origRet = len;
		ZInStream.avail_out = len;
		ZInStream.next_out = buf;

		ZInStream.avail_in = ret;
		ZInStream.next_in = Rbuf;

		int zret = inflate(&ZInStream, Z_SYNC_FLUSH);
		if(zret) {
			fprintf(stderr, "inflate failed: %d\n", zret);
			exit(1);
		}
		ret = len - ZInStream.avail_out;
	}

	return ret;
}

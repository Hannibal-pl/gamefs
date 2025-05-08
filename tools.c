#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <zlib.h>

#include "gamefs.h"

void pathDosToUnix(char *path, uint32_t len) {
	for (uint32_t i = 0; i < len; i++) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
}

void strrev(char *str, uint32_t len) {
	char tmp;
	for (uint32_t i = 0; i < (len / 2); i++) {
		tmp = str[len - 1 - i];
		str[len - 1 - i] = str[i];
		str[i] = tmp;
	}
}

time_t fatTime(uint32_t val) {
	struct tm tm;

	tm.tm_sec = (val & 0x1F) << 1;
	tm.tm_min = ((val >> 5) & 0x3F);
	tm.tm_hour = ((val >> 11) & 0x1F);
	tm.tm_mday = ((val >> 16) & 0x1F);
	tm.tm_mon = ((val >> 21) & 0x0F);
	tm.tm_year = ((val >> 25) & 0x7F) + 80;
	return mktime(&tm);
}

bool unpackSizeless(uint8_t *in, uint32_t insize, uint8_t **out, uint32_t *outsize) {
	uint32_t ret;
	uint32_t have;
	z_stream strm;
	uint8_t buf[INFLATE_CHUNK];
	uint8_t *lout = NULL;
	uint32_t loutsize = 0;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = insize;
	strm.next_in = in;
	ret = inflateInit(&strm);
	if (ret != Z_OK) {
		return false;
	}

	do {
		strm.avail_out = INFLATE_CHUNK;
		strm.next_out = buf;
		ret = inflate(&strm, Z_NO_FLUSH);
		if ((ret == Z_STREAM_ERROR) || (ret == Z_NEED_DICT) || (ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR)) {
			goto error;
		}
		have = INFLATE_CHUNK - strm.avail_out;

		lout = realloc(lout, loutsize + have);
		if (!lout) {
			goto error;
		}
		memcpy(&lout[loutsize], buf, have);
		loutsize += have;
	} while (strm.avail_out == 0);

	*out = lout;
	*outsize = loutsize;
	inflateEnd(&strm);
	return true;

error:
	inflateEnd(&strm);
	free(lout);
	return false;
}

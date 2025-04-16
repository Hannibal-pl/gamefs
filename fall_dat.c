#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <pthread.h>
#include <syslog.h>

#include "gamefs.h"

struct filenode **dirs;

unsigned char * fall_dat_uncompress_lzss(unsigned char *dest, unsigned packed, unsigned char *src) {
	unsigned char buf[4096];
	unsigned bpos = 4078;
	unsigned type = 0;
	unsigned offset, len;

	memset(buf, ' ', 4078);
	while (true) {
		type >>= 1;
		if (!(type & 0x100)) {
			type = 0xFF00 | *src++;
			if (!--packed) {
				goto exit;
			}
		}

		if (type & 1) {
			*dest++ = *src;
			buf[bpos++] = *src++;
			if (!--packed) {
				goto exit;
			}
			bpos %= 4096;
		} else {
			offset = (unsigned)*src + (((unsigned)*(src + 1) & 0xF0) << 4);
			len = (*(src + 1) & 0xF) + 3;
			src += 2;
			for (int i = 0; i < len; i++) {
				*dest++ = buf[(offset + i) % 4096];
				buf[bpos++] = buf[(offset + i) % 4096];
				bpos %= 4096;
			}

			if (!--packed) {
				goto exit;
			}
			if (!--packed) {
				goto exit;
			}
		}
	}

exit:
	return dest;
}

int fall_dat_uncompress(unsigned char *dest, unsigned packed, unsigned char *src) {
	unsigned short chunk;
	while(packed) {
		chunk = (((unsigned short)*src) << 8) + (unsigned short)*(src + 1);
		src += 2;
		packed -= 2;
		if (chunk & 0x8000) {
			chunk &= 0x7FFF;
			memcpy(dest, src, chunk);
			dest += chunk;
			src += chunk;
			packed -= chunk;
		} else {
			dest = fall_dat_uncompress_lzss(dest, chunk, src);
			src += chunk;
			packed -= chunk;
		}
	}
	return 0;
}

int fall_dat_fo_open(const char *path, struct fuse_file_info *fi) {
	struct filenode *node = NULL;
	void *packed;

	node = generic_find_path(fs->root, path);
	if (!node) {
		return -ENOENT;
	}

	if ((fi->flags & 3) != O_RDONLY) {
		return -EACCES;
	}

	if (generic_is_dir(node)) {
		return -EISDIR;
	}

	pthread_mutex_lock(&fs->mutex);
	node->data = malloc(node->size);
	packed = malloc(node->packed);
	if (!node->data || !packed) {
		if (node->data) {
			free(node->data);
			node->data = NULL;
		}
		if (packed) {
			free(packed);
		}
		pthread_mutex_unlock(&fs->mutex);
		return -ENOMEM;
	}

	fseek(fs->file, node->offset, SEEK_SET);
	if (generic_is_type(node, FILETYPE_PACKED)) {
		fread(packed, 1, node->packed, fs->file);
		fall_dat_uncompress(node->data, node->packed, packed);
	} else {
		fread(node->data, 1, node->size, fs->file);
	}

	node->open_no++;
	pthread_mutex_unlock(&fs->mutex);
	free(packed);
	return 0;
}

int init_game_fall_dat(void) {
	unsigned dircount;
	unsigned flags;
	unsigned count;
	unsigned char slen;
	char path[MAX_FILENAME];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(&dircount, 1, sizeof(unsigned), fs->file);
	dircount = be32toh(dircount);
	dirs = (struct filenode **)malloc(sizeof(struct filenode *) * dircount);
	if (!dirs) {
		fprintf(stderr, "Not enough memory for temporary directory list.\n");
		return -ENOMEM;
	}

	fseek(fs->file, 16, SEEK_SET);
	for (int i = 0; i < dircount; i++) {
		memset(path, 0, sizeof(path));
		fread(&slen, 1, sizeof(unsigned char), fs->file);
		fread(path, slen, 1, fs->file);
		if (!strcmp(path, ".")) {
			dirs[i] = fs->root;
		} else {
			pathDosToUnix(path, slen);
			dirs[i] = generic_add_path(fs->root, path, FILETYPE_DIR);
			if (!dirs[i]) {
				fprintf(stderr, "Error adding file desciption to library.\n");
				free(dirs);
				return errno;
			}
		}
	}

	for (int i = 0; i < dircount; i++) {
		fread(&count, 1, sizeof(unsigned), fs->file);
		count = be32toh(count);
		fseek(fs->file, 12, SEEK_CUR);
		for (int j = 0; j < count; j++) {
			memset(path, 0, sizeof(path));
			fread(&slen, 1, sizeof(unsigned char), fs->file);
			fread(path, slen, 1, fs->file);
			fread(&flags, 1, sizeof(unsigned), fs->file);
			if (flags == 0x40000000) {
				node = generic_add_file(dirs[i], path, FILETYPE_PACKED);
			} else {
				node = generic_add_file(dirs[i], path, FILETYPE_REGULAR);
			}
			if (!node) {
				fprintf(stderr, "Error adding file desciption to library.\n");
				free(dirs);
				return errno;
			}
			fread(&node->offset, sizeof(unsigned), 1, fs->file);
			node->offset = be32toh(node->offset);
			fread(&node->size, sizeof(unsigned), 1, fs->file);
			node->size = be32toh(node->size);
			fread(&node->packed, sizeof(unsigned), 1, fs->file);
			node->packed = be32toh(node->packed);
		}
	}

	fs->operations.open = fall_dat_fo_open;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	free(dirs);
	return 0;
}

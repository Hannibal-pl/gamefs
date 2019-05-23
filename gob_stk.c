#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#include "gob_stk.h"

int gob_stk_uncompress(unsigned char *dest, unsigned unpacked, unsigned char *src) {
	unsigned char buf[4096];
	unsigned bpos = 4078;
	unsigned type = 0;
	unsigned offset, len;

	memset(buf, ' ', 4078);
	while (true) {
		type >>= 1;
		if (!(type & 0x100)) {
			type = 0xFF00 | *src++;
		}

		if (type & 1) {
			*dest++ = *src;
			if (!--unpacked) {
				goto exit;
			}

			buf[bpos++] = *src++;
			bpos %= 4096;
		} else {
			offset = (unsigned)*src + (((unsigned)*(src + 1) & 0xF0) << 4);
			len = (*(src + 1) & 0xF) + 3;
			src += 2;
			for (int i = 0; i < len; i++) {
				*dest++ = buf[(offset + i) % 4096];
				if (!--unpacked) {
					goto exit;
				}

				buf[bpos++] = buf[(offset + i) % 4096];
				bpos %= 4096;
			}
		}
	}

exit:
	return 0;
}

int gob_stk_fo_open(const char *path, struct fuse_file_info *fi) {
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
	packed = malloc(node->packed);
	if (!packed) {
		pthread_mutex_unlock(&fs->mutex);
		return -ENOMEM;
	}

	fseek(fs->file, node->offset, SEEK_SET);
	if (generic_is_type(node, FILETYPE_PACKED)) {
		node->data = malloc(node->size);
		if (!node->data) {
			free(packed);
			pthread_mutex_unlock(&fs->mutex);
			return -ENOMEM;
		}

		fread(packed, 1, node->packed, fs->file);
		gob_stk_uncompress(node->data, node->size, packed);

		free(packed);
	} else {
		node->data = packed;
		fread(node->data, 1, node->size, fs->file);
	}

	node->open_no++;
	pthread_mutex_unlock(&fs->mutex);
	return 0;
}

int init_game_gob_stk(void) {
	unsigned offset;
	unsigned size;
	unsigned short count;
	unsigned char tmp;
	char name[13];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(&count, sizeof(unsigned short), 1, fs->file);

	for (int i = 0; i < count; i++) {
		memset(name, 0 ,sizeof(name));
		fread(name, 1, sizeof(name), fs->file);
		fread(&size, sizeof(unsigned), 1, fs->file);
		fread(&offset, sizeof(unsigned), 1, fs->file);
		fread(&tmp, sizeof(unsigned char), 1, fs->file);

		if (tmp == 1) {
			node = generic_add_file(fs->root, name, FILETYPE_PACKED);
		} else {
			node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		}
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		node->size = size;
		node->packed = size;
		node->offset = offset;
		if (tmp == 1) {
			node->packed -= 4;
			offset = ftell(fs->file);
			fseek(fs->file, node->offset, SEEK_SET);
			fread(&node->size, sizeof(unsigned), 1, fs->file);
			node->offset += 4;
			fseek(fs->file, offset, SEEK_SET);
		}
	}

	fs->operations.open = gob_stk_fo_open;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
//#include <syslog.h>

#include "../gamefs.h"

void tri2_fbq_uncompress(uint8_t *dest, uint32_t unpacked, uint8_t *src) {
	uint32_t len;
	uint32_t off;

	for (uint32_t i = 0, j = 0; i < unpacked;) {
		if (src[j] <= 0x1F) { //copy
			len = src[j++] + 1;
//			syslog(LOG_DEBUG, "Copy at   0x%04X to 0x%04X len 0x%02hhX\n", i, j, len);
			for (uint32_t k = 0; k < len; k++, i++, j++) {
				dest[i] = src[j];
			}
		} else if (src[j] <= 0xDF) { //dict short
			len = ((uint32_t)src[j] >> 5) + 2;
			off = (((uint32_t)(src[j++] & 0x1F)) << 8);
			off += (uint32_t)src[j++] + 1;
//			syslog(LOG_DEBUG, "Dict from 0x%04X to 0x%04X len 0x%02hhX\n", i - off, j, len);
			for (uint32_t k = 0; k < len; k++, i++) {
				dest[i] = dest[i - off];
			}
		} else { //dict long
			off = (((uint32_t)(src[j++] & 0x1F)) << 8);
			len = (uint32_t)(src[j++]) + 9;
			off += (uint32_t)src[j++] + 1;
//			syslog(LOG_DEBUG, "Long from 0x%04X to 0x%04X len 0x%02hhX\n", i - off, j, len);
			for (uint32_t k = 0; k < len; k++, i++) {
				dest[i] = dest[i - off];
			}
		}
	}
}


int tri2_fbq_fo_open(const char *path, struct fuse_file_info *fi) {
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
		tri2_fbq_uncompress(node->data, node->size, packed);

		free(packed);
	} else {
		node->data = packed;
		fread(node->data, 1, node->size, fs->file);
	}

	node->open_no++;
	pthread_mutex_unlock(&fs->mutex);
	return 0;
}

int init_game_tri2_fbq(void) {
	uint32_t count;
	uint32_t offset;
	uint32_t foffset;
	uint8_t unpacked;
	char magic[4];
	char name[MAX_PATH];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "\2\0\0\0", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fread(&count, sizeof(uint32_t), 1, fs->file);
	fread(&offset, sizeof(uint32_t), 1, fs->file);

	for (uint32_t i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		for (uint32_t j = 0; j < sizeof(name); j++) {
			name[j] = fgetc(fs->file);
			if (!name[j]) {
				break;
			}
		}
		if (name[MAX_PATH - 1]) {
			fprintf(stderr, "Path too long.\n");
			return -ENAMETOOLONG;
		}
		fread(&foffset, sizeof(uint32_t), 1, fs->file);
		fread(&unpacked, sizeof(uint8_t), 1, fs->file);

		if (unpacked) {
			node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		} else {
			node = generic_add_path(fs->root, name, FILETYPE_PACKED);
		}

		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		fread(&node->size, sizeof(uint32_t), 1, fs->file);
		fread(&node->packed, sizeof(uint32_t), 1, fs->file);
		fseek(fs->file, sizeof(uint32_t), SEEK_CUR);
//		node->size = node->packed; // For now. After cracking compression, it should be removed.
		node->offset = foffset + offset + 12;
	}

	fs->operations.open = tri2_fbq_fo_open;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;


	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_tri2_fbq(void) {
	char magic[4];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "\2\0\0\0", 4)) ? false : true;
}

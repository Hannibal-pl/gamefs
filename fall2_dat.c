#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "fall2_dat.h"

int init_game_fall2_dat(void) {
	unsigned offset;
	unsigned count;
	unsigned fnsize;
	unsigned char tmp;
	unsigned magicsize;
	char path[MAX_PATH];
	struct filenode *node;

	fseek(fs->file, -4, SEEK_END);
	fread(&magicsize, 1, sizeof(unsigned), fs->file);
	if (magicsize != ftell(fs->file)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, -8, SEEK_END);
	fread(&offset, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, -offset - 8, SEEK_END);
	fread(&count, sizeof(unsigned), 1, fs->file);

	for (int i = 0; i < count; i++) {
		memset(path, 0 ,sizeof(path));
		fread(&fnsize, sizeof(unsigned), 1, fs->file);
		if (fnsize >= MAX_PATH) {
			fprintf(stderr, "Path too long.\n");
			return -ENAMETOOLONG;
		}
		fread(path, fnsize, 1, fs->file);
		for (int j = 0; j < fnsize; j++) {
			if (path[j] == '\\') {
				path[j] = '/';
			}
		}
		fread(&tmp, sizeof(unsigned char), 1, fs->file);

		if (tmp == 1) {
			node = generic_add_path(fs->root, path, FILETYPE_PACKED);
		} else {
			node = generic_add_path(fs->root, path, FILETYPE_REGULAR);
		}
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->size, sizeof(unsigned), 1, fs->file);
		fread(&node->packed, sizeof(unsigned), 1, fs->file);
		fread(&node->offset, sizeof(unsigned), 1, fs->file);
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

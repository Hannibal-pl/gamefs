#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "gamefs.h"

int init_game_arc_dat(void) {
	unsigned offset;
	unsigned count;
	unsigned fnsize;
	unsigned tmp;
	unsigned char magic[4];
	char path[MAX_PATH];
	struct filenode *node;

	fseek(fs->file, -12, SEEK_END);
	fread(magic, sizeof(char), 4, fs->file);
	if (memcmp(magic, "1TAD", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, -4, SEEK_END);
	fread(&offset, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, -offset, SEEK_END);
	fread(&count, sizeof(unsigned), 1, fs->file);

	for(int i = 0; i < count; i++) {
		memset(path, 0 ,sizeof(path));
		fread(&fnsize, sizeof(unsigned), 1, fs->file);
		if (fnsize >= MAX_PATH) {
			fprintf(stderr, "Path too long.\n");
			return -ENAMETOOLONG;
		}
		fread(path, fnsize, 1, fs->file);
		pathDosToUnix(path, fnsize);
		fseek(fs->file, 4, SEEK_CUR);
		fread(&tmp, sizeof(unsigned), 1, fs->file);

		if (tmp == 2) {
			node = generic_add_path(fs->root, path, FILETYPE_PACKED);
		} else if (tmp == 1) {
			node = generic_add_path(fs->root, path, FILETYPE_REGULAR);
		} else {
			node = generic_add_path(fs->root, path, FILETYPE_DIR);
		}
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		if (generic_is_dir(node)) {
			fseek(fs->file, sizeof(unsigned) * 3, SEEK_CUR);
		} else {
			fread(&node->size, sizeof(unsigned), 1, fs->file);
			fread(&node->packed, sizeof(unsigned), 1, fs->file);
			fread(&node->offset, sizeof(unsigned), 1, fs->file);
		}
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

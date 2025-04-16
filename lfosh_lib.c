#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "gamefs.h"

int init_game_lfosh_lib(void) {
	unsigned short count;
	char name[13];
	unsigned char magic[4];
	struct filenode *node, *prev_node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "LIB\x1A", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fread(&count, sizeof(unsigned short), 1, fs->file);
	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name), fs->file);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		if (i > 0) {
			prev_node->size = node->offset - prev_node->offset;
		}
		prev_node = node;
	}
	fseek(fs->file, 0, SEEK_END);
	node->size = ftell(fs->file) - node->offset;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
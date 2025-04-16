#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "../gamefs.h"

int init_game_sc2000_dat(void) {
	unsigned count;
	char name[13];
	struct filenode *node, *prev_node;

	fseek(fs->file, 12, SEEK_SET);
	fread(&count, sizeof(unsigned), 1, fs->file);
	count >>= 4;

	fseek(fs->file, 0, SEEK_SET);
	for(int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name) - 1, fs->file);
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

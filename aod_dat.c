#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "gamefs.h"

int init_game_aod_dat(void) {
	unsigned count;
	struct filenode *node, *prev_node;

	fseek(fs->file, 0, SEEK_SET);
	fread(&count, sizeof(unsigned), 1, fs->file);
	for (int i = 0; i < count; i++) {
		node = generic_add_unknown(FILETYPE_REGULAR);
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

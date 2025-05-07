#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_bards_lmp(void) {
	uint32_t count;
	char name[57];
	struct filenode *node;

	fread(&count, sizeof(uint32_t), 1, fs->file);

	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name) - 1, fs->file);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(uint32_t), 1, fs->file);
		fread(&node->size, sizeof(uint32_t), 1, fs->file);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_bards_lmp(void) {
	return false;
}

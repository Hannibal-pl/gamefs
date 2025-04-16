#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "generic.h"

int init_game_bs_rarc(void) {
	unsigned count;
	char magic[4];
	char name[65];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "RARC", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fread(&count, sizeof(unsigned), 1, fs->file);

	for(int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name) - 1, fs->file);
		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		fread(&node->size, sizeof(unsigned), 1, fs->file);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

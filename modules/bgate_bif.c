#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "../gamefs.h"

int init_game_bgate_bif(void) {
	uint32_t count;
	uint32_t offset;
	char magic[8];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "BIFFV1  ", 8)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fread(&count, sizeof(uint32_t), 1, fs->file);
	fseek(fs->file, 4, SEEK_CUR);
	fread(&offset, sizeof(uint32_t), 1, fs->file);
	fseek(fs->file, offset, SEEK_SET);

	for (int i = 0; i < count; i++) {
		node = generic_add_unknown(FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		fseek(fs->file, 4, SEEK_CUR);
		fread(&node->offset, sizeof(uint32_t), 1, fs->file);
		fread(&node->size, sizeof(uint32_t), 1, fs->file);
		fseek(fs->file, 4, SEEK_CUR);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

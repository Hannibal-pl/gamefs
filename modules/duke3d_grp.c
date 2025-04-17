#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_duke3d_grp(void) {
	uint32_t count;
	uint32_t offset;
	char magic[12];
	char name[13];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "KenSilverman", 12)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fread(&count, sizeof(uint32_t), 1, fs->file);
	offset = 16 + 16 * count;

	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 12, 1, fs->file);

		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		fread(&node->size, sizeof(uint32_t), 1, fs->file);
		node->offset = offset;

		offset += node->size;
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

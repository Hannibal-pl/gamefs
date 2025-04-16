#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_as688_mlb(void) {
	unsigned short count;
	unsigned short magic;
	unsigned offset, size;
	char name[13];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(&count, sizeof(unsigned short), 1, fs->file);
	fread(&magic, sizeof(unsigned short), 1, fs->file);
	if (magic != 6) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(&offset, sizeof(unsigned), 1, fs->file);
		fread(&size, sizeof(unsigned), 1, fs->file);
		fread(name, 1, sizeof(name), fs->file);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		node->offset = offset;
		node->size = size;
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
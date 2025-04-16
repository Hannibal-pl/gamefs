#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_fragall__() {
	unsigned count;
	char name[16];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(name, 1, sizeof(name), fs->file);
	if (strncmp(fs->options.file, name, sizeof(name)) != 0) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, sizeof(unsigned), SEEK_CUR);
	fread(&count, sizeof(unsigned), 1, fs->file);
	count = (count / 24) - 1;
	printf("%i\n", count);

	for(int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name), fs->file);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
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
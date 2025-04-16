#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_ftl_dat(void) {
	unsigned count;
	char path[MAX_PATH];
	struct filenode *node;
	int size;
	int offset;
	int len;

	fseek(fs->file, 4, SEEK_SET);
	fread(&count, 1, sizeof(int), fs->file);
	count = (count - 4) / sizeof(int);
	fseek(fs->file, 4, SEEK_SET);

	for(int i = 0; i < count; i++) {
		memset(path, 0, sizeof(path));
		fseek(fs->file, 4 + (i * sizeof(int)), SEEK_SET);
		fread(&offset, 1, sizeof(int), fs->file);
		if (offset == 0) {
			continue;
		}

		fseek(fs->file, offset, SEEK_SET);
		fread(&size, 1, sizeof(int), fs->file);
		fread(&len, 1, sizeof(int), fs->file);
		offset += 8 + len;
		if (len > (MAX_PATH - 1)) {
			fprintf(stderr, "Too long path.\n");
		}

		fread(path, 1, len, fs->file);
		path[len] = '\0';

		node = generic_add_path(fs->root, path, FILETYPE_REGULAR);
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

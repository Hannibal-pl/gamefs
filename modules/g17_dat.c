#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "../gamefs.h"

int init_game_g17_dat(void) {
	unsigned count;
	unsigned offset, size;
	char path[113];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(&count, sizeof(unsigned), 1, fs->file);
	count--;

	fseek(fs->file, 32, SEEK_SET);
	for(int i = 0; i < count; i++) {
		memset(path, 0, sizeof(path));
		fread(&offset, sizeof(unsigned), 1, fs->file);
		fread(&size, sizeof(unsigned), 1, fs->file);
		fseek(fs->file, sizeof(unsigned) * 2, SEEK_CUR);
		fread(path, 1, sizeof(path) - 1, fs->file);
		pathDosToUnix(path, strlen(path));
		node = generic_add_path(fs->root, path, FILETYPE_REGULAR);
		if (!node) {
			if (errno == -EEXIST) { //ignore duplicates
				continue;
			}
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		node->offset = offset;
		node->size = size;
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_g17_dat(void) {
	return false;
}

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "gamefs.h"

int init_game_tlj_xarc(void) {
	unsigned count;
	unsigned offset;
	char magic[4];
	char name[MAX_FILENAME];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "\1\0\0\0", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fread(&count, sizeof(unsigned), 1, fs->file);
	fread(&offset, sizeof(unsigned), 1, fs->file);

	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		for (int j = 0; j < sizeof(name); j++) {
			name[j] = fgetc(fs->file);
			if (!name[j]) {
				break;
			}
		}
		if (name[MAX_FILENAME - 1]) {
			fprintf(stderr, "Filename too long.\n");
			return -ENAMETOOLONG;
		}
		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		fread(&node->size, sizeof(unsigned), 1, fs->file);
		fseek(fs->file, sizeof(unsigned), SEEK_CUR);
		node->offset = offset + 124;
		offset += node->size;
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

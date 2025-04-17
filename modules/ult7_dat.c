#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_ult7_dat(void) {
	unsigned count;
	unsigned offset;
	unsigned size;
	char magic[40];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "Ultima VII Data File (C) 1992 Origin Inc", 40)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fseek(fs->file, 84, SEEK_SET);
	fread(&count, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, 128, SEEK_SET);

	for (int i = 0; i < count; i++) {
		fread(&offset, sizeof(unsigned), 1, fs->file);
		fread(&size, sizeof(unsigned), 1, fs->file);
		if (offset && size) {
			node = generic_add_unknown(FILETYPE_REGULAR);
			if (!node) {
				fprintf(stderr, "Error adding file desciption to library.\n");
				return errno;
			}
			node->offset = offset;
			node->size = size;
		}
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_ult7_dat(void) {
	char magic[40];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "Ultima VII Data File (C) 1992 Origin Inc", 40)) ? false : true;
}

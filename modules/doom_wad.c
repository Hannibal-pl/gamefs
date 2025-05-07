#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_doom_wad(void) {
	uint32_t count;
	uint32_t offset;
	uint32_t size;
	char magic[4];
	char name[9];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "IWAD", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fread(&count, sizeof(uint32_t), 1, fs->file);
	fread(&offset, sizeof(uint32_t), 1, fs->file);
	fseek(fs->file, offset, SEEK_SET);

	for(uint32_t i = 0; i < count; i++) {
		fread(&offset, sizeof(uint32_t), 1, fs->file);
		fread(&size, sizeof(uint32_t), 1, fs->file);
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name) - 1, fs->file);
		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
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

bool detect_game_doom_wad(void) {
	char magic[4];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "IWAD", 4)) ? false : true;
}

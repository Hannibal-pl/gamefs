#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_hotmi_wad(void) {
	uint32_t count;
	uint32_t offset;
	uint32_t ssize;
	char name[MAX_PATH];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(&offset, sizeof(uint32_t), 1, fs->file);
	fread(&count, sizeof(uint32_t), 1, fs->file);
	for (int i = 0; i < count; i++) {
		fread(&ssize, sizeof(uint32_t), 1, fs->file);
		if (ssize >= MAX_PATH) {
			fprintf(stderr, "Path too long.\n");
			return -ENAMETOOLONG;
		}
		memset(name, 0, sizeof(name));
		fread(name, ssize, 1, fs->file);

		fprintf(stderr, "%s\n", name);
		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);

		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
//		fseek(fs->file, 6, SEEK_CUR);
		fread(&node->size, sizeof(unsigned), 1, fs->file);
		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		node->offset += offset;
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

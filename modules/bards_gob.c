#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_bards_gob(void) {
	uint32_t count;
	uint32_t offset;
	char dname[33];
	char name[57];
	struct filenode *node, *dnode;

	for (int32_t i = 0;; i++) {
		fseek(fs->file, i * 40, SEEK_SET);
		memset(dname, 0, sizeof(dname));
		fread(dname, 1, sizeof(dname) - 1, fs->file);
		if (dname[0] == '\0') {
			break;
		}

		dnode = generic_add_file(fs->root, dname, FILETYPE_DIR);
		if (!node) {
			fprintf(stderr, "Error adding directory to library.\n");
			return errno;
		}
		fread(&offset, sizeof(uint32_t), 1, fs->file);
		if (offset == 0) {
			continue;
		}

		fseek(fs->file, offset, SEEK_SET);

		fread(&count, sizeof(uint32_t), 1, fs->file);

		for (int j = 0; j < count; j++) {
			memset(name, 0, sizeof(name));
			fread(name, 1, sizeof(name) - 1, fs->file);
			node = generic_add_file(dnode, name, FILETYPE_REGULAR);
			if (!node) {
				fprintf(stderr, "Error adding file desciption to library.\n");
				return errno;
			}

			fread(&node->offset, sizeof(uint32_t), 1, fs->file);
			node->offset += offset;
			fread(&node->size, sizeof(uint32_t), 1, fs->file);
		}
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_bards_gob(void) {
	return false;
}

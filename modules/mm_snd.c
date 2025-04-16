#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "../gamefs.h"

int init_game_mm_snd(void) {
	unsigned count;
	char name[40];
	bool tofix;
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(&count, sizeof(unsigned), 1, fs->file);
	for(int i = 0; i < count; i++) {
		tofix = false;
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name), fs->file);
		for(int j = 39; j >=0; j--) {
			if (tofix && (name[j] == '\0')) {
				name[j] = '.';
			} else if (name[j] != '\0') {
				tofix = true;
			}
		}
		node = generic_add_file(fs->root, name, FILETYPE_PACKED);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		fread(&node->packed, sizeof(unsigned), 1, fs->file);
		fread(&node->size, sizeof(unsigned), 1, fs->file);
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;


	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "generic.h"

int init_game_mm_vid(void) {
	unsigned count;
	char name[40];
	bool tofix;
	struct filenode *node, *prev_node;

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
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		if (i > 0) {
			prev_node->size = node->offset - prev_node->offset;
		}
		prev_node = node;
	}
	fseek(fs->file, 0, SEEK_END);
	node->size = ftell(fs->file) - node->offset;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

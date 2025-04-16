#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "../gamefs.h"

int init_game_dune2_pak(void) {
	char name[13];
	struct filenode *node, *prev_node;
	unsigned offset = 0;
	unsigned prev_offset = 0;

	fseek(fs->file, 0, SEEK_SET);
	fread(&offset, 1, sizeof(unsigned), fs->file);
	for (bool b = false; offset; b = true) {
		memset(name, 0, sizeof(name));
		for (int i = 0; i < 13; i++) {
			name[i] = fgetc(fs->file);
			if (!name[i]) {
				break;
			}
		}
		printf("%s\n", name);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			if (errno == -EEXIST) { //ignore duplicates
				prev_node->size = offset - prev_offset;
				prev_offset = offset;
				fread(&offset, 1, sizeof(unsigned), fs->file);
				continue;
			}
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		node->offset = offset;

		if (b) {
			prev_node->size = offset - prev_offset;
		}
		prev_node = node;
		prev_offset = offset;
		fread(&offset, 1, sizeof(unsigned), fs->file);
	}
	fseek(fs->file, 0, SEEK_END);
	node->size = ftell(fs->file) - prev_offset;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
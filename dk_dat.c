#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "gamefs.h"

int init_game_dk_dat(void) {
	unsigned count;
	unsigned offset, end;
	char name[17];
	struct filenode *node;

	fseek(fs->file, -4, SEEK_END);
	fread(&end, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, -60, SEEK_END);
	fread(&offset, sizeof(unsigned), 1, fs->file);
	offset += 32;
	count = (end - offset) / 32;

	fseek(fs->file, offset, SEEK_SET);
	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name) - 1, fs->file);
		printf("%s\n", name);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			if (errno == -EEXIST) { //ignore duplicates
				fseek(fs->file, 16, SEEK_CUR);
				continue;
			}
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fseek(fs->file, 2, SEEK_CUR);
		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		fseek(fs->file, 4, SEEK_CUR);
		fread(&node->size, sizeof(unsigned), 1, fs->file);
		fseek(fs->file, 2, SEEK_CUR);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
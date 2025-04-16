#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "gamefs.h"

int init_game_fez_pak(void) {
	unsigned count;
	char name[257];
	struct filenode *node;
	unsigned char nsize;
	unsigned size;

	fseek(fs->file, 0, SEEK_SET);
	fread(&count, sizeof(unsigned), 1, fs->file);

	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(&nsize, 1, sizeof(nsize), fs->file);
		fread(name, 1, nsize, fs->file);
		pathDosToUnix(name, strlen(name));

		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			if (errno != -EEXIST) {
				fprintf(stderr, "Error adding file desciption to library %s.\n", name);
				return errno;
			} //ignore duplicates
		}

		fread(&size, sizeof(unsigned), 1, fs->file);
		if (node) {
			node->offset = ftell(fs->file);
			node->size = size;
		}
		fseek(fs->file, size, SEEK_CUR);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

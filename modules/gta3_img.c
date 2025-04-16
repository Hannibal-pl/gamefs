#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "../gamefs.h"

int init_game_gta3_img(void) {
	unsigned count;
	unsigned offset, size;
	char name[25];
	struct filenode *node;
	char *filename;
	unsigned len;
	FILE *dir;

	filename = strdup(fs->options.file);
	if (!filename) {
		fprintf(stderr, "Not enough memory for dir filename.\n");
		errno = -ENOMEM;
		return -ENOMEM;
	}
	len = strlen(filename);
	filename[len - 3] = 'd';
	filename[len - 2] = 'i';
	filename[len - 1] = 'r';
	dir = fopen(filename, "rw");
	free(filename);
	if (!dir) {
		fprintf(stderr, "Directory file not found.\n");
		errno = -EINVAL;
		return -EINVAL;
	}

	fseek(dir, 0, SEEK_END);
	count = ftell(dir) / 32;
	fseek(dir, 0, SEEK_SET);

	for(int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(&offset, sizeof(unsigned), 1, dir);
		fread(&size, sizeof(unsigned), 1, dir);
		fread(name, 1, sizeof(name) - 1, dir);
		printf("%s\n", name);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			if (errno == -EEXIST) { //ignore duplicates
				continue;
			}
			fprintf(stderr, "Error adding file desciption to library.\n");
			fclose(dir);
			return errno;
		}
		node->offset = offset * 0x800;
		node->size = size * 0x800;
	}

	fclose(dir);
	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_ja2_slf(void) {
	unsigned short count;
	char path[257];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	memset(path, 0, sizeof(path));
	fread(path, 1, sizeof(path) - 1, fs->file);
	if (!strcasestr(path, fs->options.file)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, 0x200, SEEK_SET);
	fread(&count, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, -(count * 0x118), SEEK_END);

	for (int i = 0; i < (count - 1); i++) {
		memset(path, 0, sizeof(path));
		fread(path, 1, sizeof(path) - 1, fs->file);
		pathDosToUnix(path, strlen(path));
		printf("%s\n", path);
		if (!strcmp(path, "26/T")) { //broken file workaround
			fseek(fs->file, sizeof(unsigned) * 6, SEEK_CUR);
			continue;
		}
		node = generic_add_path(fs->root, path, FILETYPE_REGULAR);
		if (!node) {
			if (errno == -EEXIST) { //ignore duplicates
				fseek(fs->file, sizeof(unsigned) * 6, SEEK_CUR);
				continue;
			}
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		fread(&node->size, sizeof(unsigned), 1, fs->file);
		fseek(fs->file, sizeof(unsigned) * 4, SEEK_CUR);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
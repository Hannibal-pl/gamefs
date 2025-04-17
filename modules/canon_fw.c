#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_arch_canon_fw(void) {
	unsigned count;
	unsigned offset;
	unsigned skip = 0;
	char magic[8];
	char name[33];
	struct filenode *node;

	fseek(fs->file, -8, SEEK_END);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "USTBIND", 7)) {
		fprintf(stderr, "Invalid firmware file.\n");
		return -EINVAL;
	}

	if (fs->options.param) {
		skip = strtoul(fs->options.param, NULL, 0);
	}

	fseek(fs->file, -16, SEEK_END);
	fread(&offset, sizeof(unsigned), 1, fs->file);
	fread(&count, sizeof(unsigned), 1, fs->file);
	fseek(fs->file, offset - skip, SEEK_SET);

	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name) - 1, fs->file);
		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		fread(&node->offset, sizeof(unsigned), 1, fs->file);
		node->offset -= skip;
		fread(&node->size, sizeof(unsigned), 1, fs->file);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_arch_canon_fw(void) {
	char magic[8];

	fseek(fs->file, -8, SEEK_END);
	fread(magic, 1, sizeof(magic), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return  (memcmp(magic, "USTBIND", 7)) ? false : true;
}

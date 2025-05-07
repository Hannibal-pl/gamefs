#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_arch_elwo_res(void) {
	uint16_t count;
	uint16_t magic;
	uint32_t offset;
	char name[13];
	struct filenode *node;

	fseek(fs->file, -2, SEEK_END);
	fread(&magic, sizeof(uint16_t), 1, fs->file);
	if (magic != 0xAAAA) {
		fprintf(stderr, "Invalid resource file.\n");
		return -EINVAL;
	}
	fseek(fs->file, -10, SEEK_END);
	fread(&count, sizeof(uint16_t), 1, fs->file);
	fread(&offset, sizeof(uint32_t), 1, fs->file);
	fseek(fs->file, offset, SEEK_SET);

	for (uint32_t i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name), fs->file);
		node = generic_add_file(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		fread(&node->offset, sizeof(uint32_t), 1, fs->file);
		fread(&node->size, sizeof(uint32_t), 1, fs->file);
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_arch_elwo_res(void) {
	uint16_t magic;

	fseek(fs->file, -2, SEEK_END);
	fread(&magic, sizeof(uint16_t), 1, fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (magic == 0xAAAA) ? true : false;
}

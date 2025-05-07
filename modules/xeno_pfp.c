#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_xeno_pfp(void) {
	uint8_t slen;
	uint32_t count;
	char magic[4];
	char name[MAX_PATH];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, sizeof(char), 4, fs->file);
	if (memcmp(magic, "PFPK", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fread(&count, sizeof(uint32_t), 1, fs->file);
	for (int i = 0; i < count; i++) {
		fread(&slen, sizeof(char), 1, fs->file);
		memset(name, 0, sizeof(name));
		fread(name, sizeof(char), slen, fs->file);

		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		fread(&node->offset, sizeof(uint32_t), 1, fs->file);
		fread(&node->size, sizeof(uint32_t), 1, fs->file);

		fprintf(stderr, "0x%08X 0x%08X 0x%08X\n", node->offset, node->packed, node->size);
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);

	return 0;
}

bool detect_game_xeno_pfp(void) {
	char magic[4];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, sizeof(char), 4, fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "PFPK", 4)) ? false : true;
}

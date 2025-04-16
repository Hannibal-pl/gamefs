#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "gamefs.h"

int init_game_anox_dat(void) {
	uint32_t count;
	uint32_t offset;
	uint32_t packed;
	uint32_t size;
	char magic[4];
	char name[MAX_PATH];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "ADAT", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fread(&offset, sizeof(uint32_t), 1, fs->file);

	fseek(fs->file, 0, SEEK_END);
	count = (ftell(fs->file) - offset) / 144;

	fseek(fs->file, offset, SEEK_SET);
	for (int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 128, 1, fs->file);
		pathDosToUnix(name, strlen(name));

		fread(&offset, sizeof(uint32_t), 1, fs->file);
		fread(&size, sizeof(uint32_t), 1, fs->file);
		fread(&packed, sizeof(uint32_t), 1, fs->file);

		if (packed != 0) {
			node = generic_add_path(fs->root, name, FILETYPE_PACKED);
		} else {
			node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		}

		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		node->offset = offset;
		node->size = size;
		node->packed = packed;

		fseek(fs->file, 4, SEEK_CUR);
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

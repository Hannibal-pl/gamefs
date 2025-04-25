#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_mm_hwl(void) {
	uint32_t count;
	uint32_t diroffset;
	char magic[4];
	char name[21];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "D3DT", 4)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fread(&diroffset, sizeof(uint32_t), 1, fs->file);
	diroffset += 4;
	fseek(fs->file, 0, SEEK_END);
	count = (ftell(fs->file) - diroffset) / 24;

	for(uint32_t i = 0; i < count; i++) {
		uint32_t w, h;

		memset(name, 0, sizeof(name));
		fseek(fs->file, diroffset + i * 20, SEEK_SET);
		fread(name, 1, 20, fs->file);
		node = generic_add_file(fs->root, name, FILETYPE_PACKED);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
		fseek(fs->file, diroffset + count * 20 + i * 4, SEEK_SET);
		fread(&node->offset, sizeof(uint32_t), 1, fs->file);

		fseek(fs->file, node->offset, SEEK_SET);
		node->offset += 36;
		fread(&node->packed, sizeof(uint32_t), 1, fs->file);

		fseek(fs->file, 16, SEEK_CUR);
		fread(&w, sizeof(uint32_t), 1, fs->file);
		fread(&h, sizeof(uint32_t), 1, fs->file);
		node->size = w * h * 2;
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_mm_hwl(void) {
	char magic[4];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "D3DT", 4)) ? false : true;
}

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_h3_lod(void) {
	uint32_t count;
	uint32_t offset;
	uint32_t size;
	uint32_t packed;
	char magic[4];
	uint32_t unk;
	char name[16];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fread(&unk, 1, sizeof(uint32_t), fs->file);
	if (memcmp(magic, "LOD\x00", 4) || (unk > 0x200)) { // unk check is for differ from M&M VI+ lod files
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fread(&count, sizeof(uint32_t), 1, fs->file);

	fseek(fs->file, 92, SEEK_SET);
	for (uint32_t i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name), fs->file);
		fread(&offset, sizeof(uint32_t), 1, fs->file);
		fread(&size, sizeof(uint32_t), 1, fs->file);
		fseek(fs->file, 4, SEEK_CUR);
		fread(&packed, sizeof(uint32_t), 1, fs->file);
		node = generic_add_file(fs->root, name, (packed == size) ? FILETYPE_REGULAR : FILETYPE_PACKED);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		node->offset = offset;
		node->size = size;
		node->packed = packed;
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_h3_lod(void) {
	char magic[4];
	uint32_t unk;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fread(&unk, 1, sizeof(uint32_t), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "LOD\x00", 4) || (unk > 0x200)) ? false : true;
}

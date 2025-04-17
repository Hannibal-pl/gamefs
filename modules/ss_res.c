#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_ss_res(void) {
	short unsigned count;
	unsigned offset;
	unsigned prev_offset;
	unsigned end;
	unsigned size;
	char magic[16];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	if (memcmp(magic, "LG Res File v2\r\n", 16)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	fseek(fs->file, 0, SEEK_END);
	size = ftell(fs->file);
	fseek(fs->file, 124, SEEK_SET);
	fread(&end, sizeof(unsigned), 1, fs->file);

	if ((end + 16) == size) {
		node = generic_add_unknown(FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}
	} else {
		fread(&count, sizeof(short unsigned), 1, fs->file);

		fread(&prev_offset, sizeof(unsigned), 1, fs->file);
		for (int i = 0; i < count; i++) {
			if (i != (count - 1)) {
				fread(&offset, sizeof(unsigned), 1, fs->file);
			} else {
				offset = end;
			}

			node = generic_add_unknown(FILETYPE_REGULAR);
			if (!node) {
				fprintf(stderr, "Error adding file desciption to library.\n");
				return errno;
			}
			node->offset = offset;
			node->size = offset - prev_offset;

			prev_offset = offset;
		}

	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}

bool detect_game_ss_res(void) {
	char magic[16];

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	fseek(fs->file, 0, SEEK_SET);
	return (memcmp(magic, "LG Res File v2\r\n", 16)) ? false : true;
}
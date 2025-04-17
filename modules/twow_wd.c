#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../gamefs.h"

int init_game_twow_wd(void) {
	uint32_t offset;
	int16_t count;
	char magic[2];
	char name[MAX_PATH];
	struct filenode *node;
	uint8_t *packed_dir = NULL;
	uint8_t *unpacked_dir = NULL;
	uint32_t unpacked_size = 0;

	fseek(fs->file, -4, SEEK_END);
	fread(&offset, 1, sizeof(uint32_t), fs->file);
	fseek(fs->file, -(uint64_t)offset, SEEK_END);
	fread(magic, sizeof(char), 2, fs->file);
	if (memcmp(magic, "\x78\x9C", 2)) { //zlib stream header
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}
	fseek(fs->file, -(uint64_t)offset, SEEK_END);

	packed_dir = malloc(offset - 4);
	if (!packed_dir) {
		fprintf(stderr, "Not enough memory for directory structure.\n");
		return -ENOMEM;
	}

	fread(packed_dir, sizeof(uint8_t), (uint64_t)offset - 4UL, fs->file);
	if (!unpackSizeless(packed_dir, offset - 4, &unpacked_dir, &unpacked_size)) {
		fprintf(stderr, "Invalid directory structure %X .\n", unpacked_size);
		return -EINVAL;
	}
	free(packed_dir);

	if ((fs->options.param) && (!strncmp(fs->options.param, "dump", 4))) { //dump directory to stdout
		fwrite(unpacked_dir, unpacked_size, 1, stdout);
		fflush(stdout);
		return 0;
	}

	count = *(int16_t *)&unpacked_dir[8];
	for (int i = 10; i < unpacked_size; count--) {
		uint8_t slen = unpacked_dir[i++];
		memset(name, 0, sizeof(name));
		memcpy(name, &unpacked_dir[i], slen);
		i += slen;
		pathDosToUnix(name, slen);

		uint8_t flag = unpacked_dir[i++];
/*		if (flag == 0) {
			fprintf(stderr, "Skipping nonstandard file '%s'\n", name);
			i += 19;
			continue;
		}*/

		node = generic_add_path(fs->root, name, FILETYPE_PACKED);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			free(unpacked_dir);
			return errno;
		}

		node->offset = *((uint32_t *)&unpacked_dir[i]);
		i += 4;
		node->packed = *((uint32_t *)&unpacked_dir[i]);
		i += 4;
		node->size = *((uint32_t *)&unpacked_dir[i]);
		i += 4;

		if (flag == 0x2B) { //WTF?
			slen = unpacked_dir[i++];
			i += slen + 16;
		} else if (flag == 0x33) {
			i += 20;
		} else if (flag == 0x39) {
			i += 40;
		} else if (flag == 0x3B) {
			slen = unpacked_dir[i++];
			i += slen + 20;
		}

		fprintf(stderr, "0x%08X 0x%08X 0x%08X\n", node->offset, node->packed, node->size);
	}

	fs->operations.open = generic_fo_open_zlib;
	fs->operations.release = generic_fo_release_mem;
	fs->operations.read = generic_fo_read_mem;

	fs->fs_size = generic_subtree_size(fs->root);

	free(unpacked_dir);
	return 0;
}

bool detect_game_twow_wd(void) {
	return false;
}

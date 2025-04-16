#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "generic.h"

int init_game_artifex_cub(void) {
	bool is64bit = false;
	unsigned count;
	char magic[8];
	char name[257];
	struct filenode *node;

	fseek(fs->file, 0, SEEK_SET);
	fread(magic, 1, sizeof(magic), fs->file);
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 8; j++) {
			magic[j] ^= i;
		}
		if (!memcmp(magic, "cub\0001.0\000", 8) || !memcmp(magic, "cub\0001.1\000", 8)) {
			generic_xor = i;
			break;
		}
		for (int j = 0; j < 8; j++) {
			magic[j] ^= i;
		}
	}

	if (memcmp(magic, "cub\0001.0\000", 8) && memcmp(magic, "cub\0001.1\000", 8)) {
		fprintf(stderr, "Invalid game file.\n");
		return -EINVAL;
	}

	if (magic[6] == '1') {
		is64bit = true;
	}

	fread(&count, sizeof(unsigned), 1, fs->file);
	for (int i = 0; i < sizeof(unsigned)/sizeof(unsigned char); i++) {
		((unsigned char *)(&count))[i] ^= generic_xor;
	}
	fseek(fs->file, 0x100, SEEK_CUR);

	if (count == 0) {
		count = 1;
	}

	for(int i = 0; i < count; i++) {
		memset(name, 0, sizeof(name));
		fread(name, 1, sizeof(name) - 1, fs->file);
		for (int j = 0; j < sizeof(name) - 1; j++) {
			name[j] ^= generic_xor;
		}
		node = generic_add_path(fs->root, name, FILETYPE_REGULAR);
		if (!node) {
			fprintf(stderr, "Error adding file desciption to library.\n");
			return errno;
		}

		if (is64bit) {
			unsigned long long ofsi;

			fread(&ofsi, sizeof(unsigned long long), 1, fs->file);
			for (int j = 0; j < sizeof(unsigned long long)/sizeof(unsigned char); j++) {
				((unsigned char *)(&ofsi))[j] ^= generic_xor;
			}
			node->offset = ofsi;

			fread(&ofsi, sizeof(unsigned long long), 1, fs->file);
			for (int j = 0; j < sizeof(unsigned long long)/sizeof(unsigned char); j++) {
				((unsigned char *)(&ofsi))[j] ^= generic_xor;
			}
			node->size = ofsi;
		} else {
			fread(&node->offset, sizeof(unsigned), 1, fs->file);
			for (int j = 0; j < sizeof(unsigned)/sizeof(unsigned char); j++) {
				((unsigned char *)(&node->offset))[j] ^= generic_xor;
			}
			fread(&node->size, sizeof(unsigned), 1, fs->file);
			for (int j = 0; j < sizeof(unsigned)/sizeof(unsigned char); j++) {
				((unsigned char *)(&node->size))[j] ^= generic_xor;
			}
		}
	}

	fs->fs_size = generic_subtree_size(fs->root);
	return 0;
}
